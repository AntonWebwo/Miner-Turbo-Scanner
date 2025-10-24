# MTS (Miner Turbo Scanner)

![Logo](https://github.com/AntonWebwo/Miner-Turbo-Scanner/blob/main/MFAvalon64.png?raw=true) 

## Описание

Это MTS — быстрый сканер для майнинг оборудования. Я написал его, чтобы быстро находить майнеры в сети. Представьте: сканируете 254 адреса за 0.5-0.7 секунды — это реально ускоряет работу с большим количеством устройств. Программа сама решает, сколько потоков использовать, и сканирует всё параллельно. Результаты приходят в удобном JSON формате. Работает через сокеты на порту 4028, что стандартно для API майнеров вроде AvalonMiner.

В проекте предусмотрена защита от переполнения стека и буфера, чтобы всё работало стабильно и безопасно.

## Возможности

- **Быстрое сканирование**: За считанные секунды проверяет целую подсеть. Идеально, когда у вас сотни устройств.
- **Параллельная работа**: Автоматически подбирает количество потоков, чтобы не нагружать систему, но и не медлить.
- **Гибкие команды**: Поддерживает кучу команд API майнеров — от простых, как "version" или "stats", до сложных, как настройка пулов или привилегированные режимы (осторожно, может сломать гарантию!).
- **JSON вывод**: Всё чисто и структурировано, легко парсить в скриптах или программах.
- **Таймауты**: Можно настроить время ожидания соединения и чтения, чтобы адаптировать под вашу сеть.
- **Кросс-платформенность**: Собрана для Windows 10 (где и тестировалась), но код можно скомпилировать под Linux.
- **Безопасность**: Защита от переполнения стека и буфера для надёжной работы.

Программа фокусируется на майнерах с API на порту 4028, так что если у вас ASIC майнеры, это то, что надо.

## Установка и Сборка

Скачайте бинарник для Windows с релизов (пока что только для Win10). Если хотите собрать сами:

- Клонируйте репозиторий.
- Убедитесь, что у вас установлен компилятор (например, GCC для Linux или MinGW для Windows).
- Соберите вручную. Для Windows используйте команду:

  ```
  g++ MTS.cpp -o MTS.exe -pthread -static -O3 -lws2_32
  ```

  Для Linux: `g++ MTS.cpp -o MTS -pthread -static -O3`

## Использование

Запускайте из командной строки. Вот базовые примеры:

### Помощь
```
C:\>MTS.exe --help

 Miner Turbo Scaner v2.0.1 Final
 Author: Anton Vinogradov @vinantole
--------------------------------------------------------------------
 A fast, multithreaded network scanner to query miner APIs.
--------------------------------------------------------------------

Usage:
  MTS.exe <target_ip> [options]                (Scan a single IP address)
  MTS.exe <start_ip> <end_ip> [options]        (Scan a range of IP addresses)

Options:
  --command <cmd>         API command to execute (default: stats).
                          For commands with parameters, use 'command|parameter' format.
                          IMPORTANT: Always enclose complex commands in double quotes!
                          Example: --command "ascset|0,led,0-1"

  --connect-timeout <ms>  Connection timeout in milliseconds (default: 500).
  --read-timeout <ms>     Read timeout in milliseconds (default: 1000).
  -h, --help              Show this detailed help message.

------------------------- AVAILABLE API COMMANDS -------------------------

[+] Simple Commands (no parameters):
    version, config, summary, pools, devs, edevs, pgacount, quit, notify,
    privileged, devdetails, stats, estats, coin, asccount, lcd

[+] Complex Commands (format: --command "command|parameter,value,...")

  --- Pool Management ---
    "switchpool|N"        (Switch to pool N)
    "enablepool|N"        (Enable pool N)
    "disablepool|N"       (Disable pool N)
    "poolpriority|N,N..." (Set pool priorities, e.g., "poolpriority|0,1,2")

  --- Device & System ---
    "asc|N"               (Get details of ASC N)
    "ascenable|N"         (Enable ASC N)
    "ascdisable|N"        (Disable ASC N)
    "ascset|N,opt[,val]"  (Set options for ASC N)
        Examples: "ascset|0,led,1-1", "ascset|0,reboot", "ascset|0,ip,s,192.168.1.100,255.255.255.0,192.168.1.1"

  --- Privileged Mode (Use with caution! May void warranty!) ---
    "privilege|0,enable_privilege,0"  (Enter privileged mode)
    "privilege|0,setvolt,<mV/10>"       (e.g., "privilege|0,setvolt,1260" for 12.6V)
    "privilege|0,setfreq,f1,f2,f3,f4,N" (Set frequency for hash board N)
    "privilege|0,settemp,<C>"           (Set target average temperature)
    "privilege|0,disable_fan"         (Disable fan self-regulation)
    "privilege|0,enable_fan"          (Enable fan self-regulation)
    "privilege|0,savesettings"        (Save current settings to flash)
    "privilege|0,get_DH"              (Get hardware error rate)
```

### Скан одного IP
```
C:\>MTS.exe 10.128.1.1 --command version
{
    "results": [
        {
            "ip": "10.128.1.1",
            "port": 4028,
            "version": {
                "STATUS": [
                    {
                        "Code": 22,
                        "Description": "cgminer 4.11.1",
                        "Msg": "CGMiner versions",
                        "STATUS": "S",
                        "When": 6147
                    }
                ],
                "VERSION": [
                    {
                        "API": "3.7",
                        "CGMiner": "4.11.1",
                        "DNA": "02010000b0ac9895",
                        "HWTYPE": "MM3v2_X3",
                        "LOADER": "d0d779de.00",
                        "MAC": "b4a2eb35c1a7",
                        "MODEL": "1246-83",
                        "PROD": "AvalonMiner 1246-83",
                        "STM8": "20.08.01",
                        "SWTYPE": "MM314",
                        "UPAPI": "2",
                        "VERSION": "21042001_4ec6bb0_61407fa"
                    }
                ],
                "id": 1
            }
        }
    ],
    "scan_summary": {
        "hosts_found": 1,
        "total_hosts_scanned": 1,
        "total_scan_time_seconds": 0.1109921
    }
}
```

### Скан диапазона
```
C:\>MTS.exe 10.128.1.0 10.128.1.10 --command version
{
    "results": [
        {
            "ip": "10.128.1.1",
            "port": 4028,
            "version": {
                "STATUS": [
                    {
                        "Code": 22,
                        "Description": "cgminer 4.11.1",
                        "Msg": "CGMiner versions",
                        "STATUS": "S",
                        "When": 6191
                    }
                ],
                "VERSION": [
                    {
                        "API": "3.7",
                        "CGMiner": "4.11.1",
                        "DNA": "02010000b0ac9895",
                        "HWTYPE": "MM3v2_X3",
                        "LOADER": "d0d779de.00",
                        "MAC": "b4a2eb35c1a7",
                        "MODEL": "1246-83",
                        "PROD": "AvalonMiner 1246-83",
                        "STM8": "20.08.01",
                        "SWTYPE": "MM314",
                        "UPAPI": "2",
                        "VERSION": "21042001_4ec6bb0_61407fa"
                    }
                ],
                "id": 1
            }
        },
        {
            "ip": "10.128.1.6",
            "port": 4028,
            "version": {
                "STATUS": [
                    {
                        "Code": 22,
                        "Description": "cgminer 4.11.1",
                        "Msg": "CGMiner versions",
                        "STATUS": "S",
                        "When": 7026
                    }
                ],
                "VERSION": [
                    {
                        "API": "3.7",
                        "CGMiner": "4.11.1",
                        "DNA": "02010000b70dcf51",
                        "HWTYPE": "MM3v2_X3",
                        "LOADER": "d0d779de.00",
                        "MAC": "b4a2eb3c8633",
                        "MODEL": "1246-N-83",
                        "PROD": "AvalonMiner 1246-N-83",
                        "STM8": "20.08.01",
                        "SWTYPE": "MM315",
                        "UPAPI": "2",
                        "VERSION": "22082901_2164eee_f95bc12"
                    }
                ],
                "id": 1
            }
        },
        {
            "ip": "10.128.1.5",
            "port": 4028,
            "version": {
                "STATUS": [
                    {
                        "Code": 22,
                        "Description": "cgminer 4.11.1",
                        "Msg": "CGMiner versions",
                        "STATUS": "S",
                        "When": 7024
                    }
                ],
                "VERSION": [
                    {
                        "API": "3.7",
                        "CGMiner": "4.11.1",
                        "DNA": "020100009cc8049f",
                        "HWTYPE": "MM3v2_X3",
                        "LOADER": "d0d779de.00",
                        "MAC": "b4a2eb3c7f99",
                        "MODEL": "1246-N-90",
                        "PROD": "AvalonMiner 1246-N-90",
                        "STM8": "20.08.01",
                        "SWTYPE": "MM315",
                        "UPAPI": "2",
                        "VERSION": "22061301_be77c30_ef5defc"
                    }
                ],
                "id": 1
            }
        },
        {
            "ip": "10.128.1.2",
            "port": 4028,
            "version": {
                "STATUS": [
                    {
                        "Code": 22,
                        "Description": "cgminer 4.11.1",
                        "Msg": "CGMiner versions",
                        "STATUS": "S",
                        "When": 6979
                    }
                ],
                "VERSION": [
                    {
                        "API": "3.7",
                        "CGMiner": "4.11.1",
                        "DNA": "02010000e40e97c9",
                        "HWTYPE": "MM3v2_X3",
                        "LOADER": "d0d779de.00",
                        "MAC": "b4a2eb35d723",
                        "MODEL": "1246-83",
                        "PROD": "AvalonMiner 1246-83",
                        "STM8": "20.08.01",
                        "SWTYPE": "MM314",
                        "UPAPI": "2",
                        "VERSION": "21072802_4ec6bb0_211fc46"
                    }
                ],
                "id": 1
            }
        },
        {
            "ip": "10.128.1.8",
            "port": 4028,
            "version": {
                "STATUS": [
                    {
                        "Code": 22,
                        "Description": "cgminer 4.11.1",
                        "Msg": "CGMiner versions",
                        "STATUS": "S",
                        "When": 6191
                    }
                ],
                "VERSION": [
                    {
                        "API": "3.7",
                        "CGMiner": "4.11.1",
                        "DNA": "0201000064b15288",
                        "HWTYPE": "MM3v2_X3",
                        "LOADER": "d0d779de.00",
                        "MAC": "b4a2eb353056",
                        "MODEL": "1246-83",
                        "PROD": "AvalonMiner 1246-83",
                        "STM8": "20.08.01",
                        "SWTYPE": "MM314",
                        "UPAPI": "2",
                        "VERSION": "21042001_4ec6bb0_61407fa"
                    }
                ],
                "id": 1
            }
        }
    ],
    "scan_summary": {
        "hosts_found": 5,
        "total_hosts_scanned": 11,
        "total_scan_time_seconds": 0.555367
    }
}
```

## Автор

Антон Виноградов, 2025  
Telegram: [@vinantole](https://t.me/vinantole)

Если есть вопросы или идеи — пишите! Проект открытый, так что контрибьюшн приветствуется.
