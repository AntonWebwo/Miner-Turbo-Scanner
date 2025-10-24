#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include "nlohmann/json.hpp"

using json = nlohmann::json;

// Global settings
int connect_timeout_ms = 500;
int read_timeout_ms = 1000;
std::string api_command = "stats";

struct Task {
    std::string ip;
    int port;
    std::string command;
};

std::queue<Task> tasks_queue;
std::mutex queue_mutex;
std::condition_variable condition;
bool done = false;

struct ScanResult {
    std::string ip;
    int port;
    bool open;
    json data;
};

std::queue<ScanResult> results_queue;
std::mutex results_mutex;

void show_help(const char* app_name) {
    std::cout << "\n Miner Turbo Scaner v2.0.1 Final\n";
    std::cout << " Author: Anton Vinogradov @vinantole\n";
    std::cout << "--------------------------------------------------------------------\n";
    std::cout << " A fast, multithreaded network scanner to query miner APIs.\n";
    std::cout << "--------------------------------------------------------------------\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << app_name << " <target_ip> [options]                (Scan a single IP address)\n";
    std::cout << "  " << app_name << " <start_ip> <end_ip> [options]        (Scan a range of IP addresses)\n\n";
    std::cout << "Options:\n";
    std::cout << "  --command <cmd>         API command to execute (default: stats).\n";
    std::cout << "                          For commands with parameters, use 'command|parameter' format.\n";
    std::cout << "                          IMPORTANT: Always enclose complex commands in double quotes!\n";
    std::cout << "                          Example: --command \"ascset|0,led,0-1\"\n\n";
    std::cout << "  --connect-timeout <ms>  Connection timeout in milliseconds (default: 500).\n";
    std::cout << "  --read-timeout <ms>     Read timeout in milliseconds (default: 1000).\n";
    std::cout << "  -h, --help              Show this detailed help message.\n\n";
    std::cout << "------------------------- AVAILABLE API COMMANDS -------------------------\n";
    std::cout << "\n[+] Simple Commands (no parameters):\n";
    std::cout << "    version, config, summary, pools, devs, edevs, pgacount, quit, notify,\n";
    std::cout << "    privileged, devdetails, stats, estats, coin, asccount, lcd\n\n";
    std::cout << "[+] Complex Commands (format: --command \"command|parameter,value,...\")\n\n";
    std::cout << "  --- Pool Management ---\n";
    std::cout << "    \"switchpool|N\"        (Switch to pool N)\n";
    std::cout << "    \"enablepool|N\"        (Enable pool N)\n";
    std::cout << "    \"disablepool|N\"       (Disable pool N)\n";
    std::cout << "    \"poolpriority|N,N...\" (Set pool priorities, e.g., \"poolpriority|0,1,2\")\n\n";
    std::cout << "  --- Device & System ---\n";
    std::cout << "    \"asc|N\"               (Get details of ASC N)\n";
    std::cout << "    \"ascenable|N\"         (Enable ASC N)\n";
    std::cout << "    \"ascdisable|N\"        (Disable ASC N)\n";
    std::cout << "    \"ascset|N,opt[,val]\"  (Set options for ASC N)\n";
    std::cout << "        Examples: \"ascset|0,led,1-1\", \"ascset|0,reboot\", \"ascset|0,ip,s,192.168.1.100,255.255.255.0,192.168.1.1\"\n\n";
    std::cout << "  --- Privileged Mode (Use with caution! May void warranty!) ---\n";
    std::cout << "    \"privilege|0,enable_privilege,0\"  (Enter privileged mode)\n";
    std::cout << "    \"privilege|0,setvolt,<mV/10>\"       (e.g., \"privilege|0,setvolt,1260\" for 12.6V)\n";
    std::cout << "    \"privilege|0,setfreq,f1,f2,f3,f4,N\" (Set frequency for hash board N)\n";
    std::cout << "    \"privilege|0,settemp,<C>\"           (Set target average temperature)\n";
    std::cout << "    \"privilege|0,disable_fan\"         (Disable fan self-regulation)\n";
    std::cout << "    \"privilege|0,enable_fan\"          (Enable fan self-regulation)\n";
    std::cout << "    \"privilege|0,savesettings\"        (Save current settings to flash)\n";
    std::cout << "    \"privilege|0,get_DH\"              (Get hardware error rate)\n\n";
}

void worker() {
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [] { return !tasks_queue.empty() || done; });
            if (done && tasks_queue.empty()) {
                return;
            }
            task = tasks_queue.front();
            tasks_queue.pop();
        }

        ScanResult result;
        result.ip = task.ip;
        result.port = task.port;
        result.open = false;

        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            {
                std::lock_guard<std::mutex> lock(results_mutex);
                results_queue.push(result);
            }
            continue;
        }

        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(task.port);
        inet_pton(AF_INET, task.ip.c_str(), &hint.sin_addr);

#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
#else
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

        connect(sock, (sockaddr*)&hint, sizeof(hint));

        fd_set fdset;
        FD_ZERO(&fdset);
        FD_SET(sock, &fdset);
        timeval tv;
        tv.tv_sec = connect_timeout_ms / 1000;
        tv.tv_usec = (connect_timeout_ms % 1000) * 1000;

        if (select(sock + 1, NULL, &fdset, NULL, &tv) == 1) {
            int so_error;
            socklen_t len = sizeof(so_error);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);
            if (so_error == 0) {
                result.open = true;

                std::string command_str = task.command;
                std::string parameter_str = "";
                size_t pipe_pos = command_str.find('|');
                if (pipe_pos != std::string::npos) {
                    parameter_str = command_str.substr(pipe_pos + 1);
                    command_str = command_str.substr(0, pipe_pos);
                }

                std::string request_payload;
                if (parameter_str.empty()) {
                    request_payload = "{\"command\":\"" + command_str + "\"}";
                } else {
                    request_payload = "{\"command\":\"" + command_str + "\",\"parameter\":\"" + parameter_str + "\"}";
                }
                
                send(sock, request_payload.c_str(), request_payload.length(), 0);

                std::string response_str;
                char buffer[4096];
                timeval read_tv;
                read_tv.tv_sec = read_timeout_ms / 1000;
                read_tv.tv_usec = (read_timeout_ms % 1000) * 1000;

                fd_set read_fdset;
                while (true) {
                    FD_ZERO(&read_fdset);
                    FD_SET(sock, &read_fdset);
                    int activity = select(sock + 1, &read_fdset, NULL, NULL, &read_tv);
                    if (activity <= 0) break;

                    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
                    if (bytes_received <= 0) break;

                    buffer[bytes_received] = '\0';
                    response_str += buffer;
                }

                try {
                    result.data = json::parse(response_str);
                } catch (const json::parse_error& e) {
                    result.data = response_str;
                }
            }
        }

        closesocket(sock);

        {
            std::lock_guard<std::mutex> lock(results_mutex);
            results_queue.push(result);
        }
    }
}

std::vector<std::string> get_ips_from_range(const std::string& start_ip, const std::string& end_ip) {
    std::vector<std::string> ips;
    unsigned int start = ntohl(inet_addr(start_ip.c_str()));
    unsigned int end = ntohl(inet_addr(end_ip.c_str()));

    if (start > end) {
        return {};
    }

    for (unsigned int i = start; i <= end; ++i) {
        in_addr addr;
        addr.s_addr = htonl(i);
        ips.push_back(inet_ntoa(addr));
    }
    return ips;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }
#endif

    std::string start_ip;
    std::string end_ip;
    std::vector<std::string> args(argv + 1, argv + argc);

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "-h" || args[i] == "--help") {
            show_help(argv[0]);
            return 0;
        } else if (args[i] == "--command") {
            if (i + 1 < args.size()) {
                api_command = args[++i];
            } else {
                std::cerr << "Error: --command requires a value." << std::endl;
                return 1;
            }
        } else if (args[i] == "--connect-timeout") {
            if (i + 1 < args.size()) {
                try {
                    connect_timeout_ms = std::stoi(args[++i]);
                } catch (const std::invalid_argument&) {
                    std::cerr << "Error: Invalid value for --connect-timeout." << std::endl;
                    return 1;
                }
            } else {
                 std::cerr << "Error: --connect-timeout requires a value." << std::endl;
                 return 1;
            }
        } else if (args[i] == "--read-timeout") {
            if (i + 1 < args.size()) {
                 try {
                    read_timeout_ms = std::stoi(args[++i]);
                } catch (const std::invalid_argument&) {
                    std::cerr << "Error: Invalid value for --read-timeout." << std::endl;
                    return 1;
                }
            } else {
                 std::cerr << "Error: --read-timeout requires a value." << std::endl;
                 return 1;
            }
        } else if (start_ip.empty()) {
            start_ip = args[i];
        } else if (end_ip.empty()) {
            end_ip = args[i];
        } else {
            std::cerr << "Error: Too many IP arguments. Provide one IP for a single target or two for a range." << std::endl;
            return 1;
        }
    }

    if (start_ip.empty()) {
        show_help(argv[0]);
        return 1;
    }

    auto total_start_time = std::chrono::steady_clock::now();

    std::vector<std::string> ips;
    if (end_ip.empty()) {
        ips.push_back(start_ip);
    } else {
        ips = get_ips_from_range(start_ip, end_ip);
    }

    if (ips.empty()) {
        std::cerr << "Error: Invalid IP range (start_ip might be greater than end_ip)." << std::endl;
        return 1;
    }

    const int port = 4028;
    const size_t total_tasks = ips.size();

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        for (const auto& ip : ips) {
            tasks_queue.push({ip, port, api_command});
        }
    }

    unsigned int num_threads = 255;
    if (total_tasks < num_threads) {
        num_threads = total_tasks;
    }

    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }
    
    size_t last_processed_count = 0;
    auto last_update_time = std::chrono::steady_clock::now();

    while (true) {
        size_t processed_count;
        {
            std::lock_guard<std::mutex> lock(results_mutex);
            processed_count = results_queue.size();
        }

        if (processed_count == total_tasks) {
            break;
        }

        if (processed_count > last_processed_count) {
            last_processed_count = processed_count;
            last_update_time = std::chrono::steady_clock::now();
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_update_time).count() > 5) {
             break; // Timeout if no new results for 5 seconds
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        done = true;
    }
    condition.notify_all();

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    json final_json;
    json results_array = json::array();
    
    std::string base_command = api_command;
    size_t pipe_pos = api_command.find('|');
    if (pipe_pos != std::string::npos) {
        base_command = api_command.substr(0, pipe_pos);
    }

    while (!results_queue.empty()) {
        ScanResult res = results_queue.front();
        results_queue.pop();
        if (res.open && !res.data.is_null() && !res.data.empty()) {
            json j;
            j["ip"] = res.ip;
            j["port"] = res.port;
            j[base_command] = res.data;
            results_array.push_back(j);
        }
    }

    final_json["results"] = results_array;

    auto total_end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> total_duration = total_end_time - total_start_time;
    
    json summary;
    summary["total_scan_time_seconds"] = total_duration.count();
    summary["hosts_found"] = results_array.size();
    summary["total_hosts_scanned"] = total_tasks;
    final_json["scan_summary"] = summary;

    std::cout << final_json.dump(4) << std::endl;

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
