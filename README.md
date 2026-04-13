# Smart Study Environment Monitor

CG2271 Real-Time Operating Systems — AY2025/26 Semester 2 — B01-08

A dual-board IoT system that monitors study desk conditions (temperature, humidity, light, noise, sitting distance) and provides real-time feedback via RGB LED, LCD, buzzer, and Telegram notifications with Gemini-generated study advisories.

## Architecture

```
                         UART                 HTTPS
MCXC444 (FreeRTOS)    ◀──────▶    ESP32    ◀───────▶    Cloud
──────────────────                ─────                  ─────
Photoresistor (ADC)               DHT11                  Firebase RTDB
Big Sound (IRQ)                   HC-SR04                Telegram Bot
RGB LED                           LCD 16x2               Gemini API
SW2/SW3 (IRQ)                     Buzzer
                                  Wi-Fi
```

## Hardware

| Component | Board | Interface |
|---|---|---|
| DHT11 (temp/humidity) | ESP32 | GPIO 4 |
| Photoresistor (light) | MCXC444 | ADC |
| Big Sound (noise) | MCXC444 | GPIO IRQ |
| HC-SR04 (distance) | ESP32 | TRIG 5 / ECHO 18 |
| RGB LED | MCXC444 | GPIO |
| Active Buzzer | ESP32 | GPIO 9 |
| LCD QC1602A + PCF8574 | ESP32 | I2C |
| SW2 (start/stop) | MCXC444 | IRQ |
| SW3 (alert ack) | MCXC444 | IRQ |

## MCXC444 — FreeRTOS Tasks

| Task | Priority | Role |
|---|---|---|
| vAlertTask | High | Reads sensor queue, sets warning level, drives RGB LED |
| vButtonTask | High | Handles SW2/SW3 interrupts |
| vCommsTask | Medium | UART TX/RX with ESP32 |
| vEnvTask | Low | Polls photoresistor and sound sensor |
| vDistanceTask | Low | Polls HC-SR04 ultrasonic sensor |

RTOS primitives: `xSensorQueue` (queue), `xSoundSemaphore` (binary semaphore), `xSystemStatusMutex` (mutex).

## ESP32 — Cloud Integration

| Module | File | Function |
|---|---|---|
| Wi-Fi | `wifi_manager.cpp` | Connect and auto-reconnect (5 s cooldown) |
| Firebase RTDB | `firebase_client.cpp` | PUT `/latest.json`, POST `/readings.json` every 1 s |
| Telegram Bot | `telegram_bot.cpp` | Command handler + push alerts (3 s poll) |
| Gemini API | `gemini_client.cpp` | Study advisory from sensor context (5 s cooldown) |
| UART RX | `uart_receive.cpp` | Parse `$MCXC` frames from MCXC444 |
| Sensors | `sensors.cpp` | DHT11 + HC-SR04 read + threshold evaluation |
| Display | `display.cpp` | LCD 16x2 output of readings and state |
| Comms | `comms.cpp` | Shared UART helpers |

Gemini is called **only** when the user sends `/ask` on Telegram or when the warning state escalates to RED/RED\_BUZZER.

## UART Protocol (9600 baud)

MCXC444 → ESP32:
```
$MCXC,<light>,<sound>,<systemActive>,<warningSuppressed>\r\n
```

ESP32 → MCXC444:
```
$ESP,<activeCount>,<temp_x10>,<dist_x10>\r\n
```

`temp_x10` and `dist_x10` are integers (temperature and distance multiplied by 10) to avoid float transmission.

## Warning States

| Code | Name | Condition | Response |
|---|---|---|---|
| 0 | IDLE | All normal | RGB Green |
| 1 | ACKNOWLEDGED | Alert dismissed via SW3 | RGB Green (alerts suppressed) |
| 2 | YELLOW | One threshold exceeded | RGB Yellow |
| 3 | RED | Multiple thresholds exceeded | RGB Red + Telegram alert + Gemini advisory |
| 4 | RED\_BUZZER | Prolonged RED | RGB Red + Buzzer + Telegram alert + Gemini advisory |

Thresholds (configurable in `config.h`): temp > 25 °C, distance < 15 cm, light < 100 (dark) or > 210 (bright), sound peak-to-peak > 9.

## Telegram Commands

Send these to your bot (only messages from the authorized `TELEGRAM_CHAT_ID` are accepted):

| Command | Example | Action |
|---|---|---|
| `/start` | `/start` | Welcome message + command list |
| `/status` | `/status` | Current `DeskState` readings |
| `/ask <question>` | `/ask is my desk too dark?` | Sensor-contextual advice from Gemini |
| `/settemp <C>` | `/settemp 28` | Echo new temp threshold (MCXC push pending) |
| `/setdist <cm>` | `/setdist 20` | Echo new distance threshold |
| `/help` | `/help` | List commands |

Auto-push: on rising edge into RED/RED\_BUZZER (and not suppressed), the bot sends 🚨 alert + 🤖 Gemini advisory.

## ESP32 Setup

1. Install [Arduino IDE](https://www.arduino.cc/en/software) and add ESP32 board support.
2. Install libraries: `ArduinoJson` (v7+), `DHT sensor library` (Adafruit), `Adafruit Unified Sensor`, `LiquidCrystal I2C`.
3. Copy `source/main/secrets.h.example` to `source/main/secrets.h` and fill in:
   - Wi-Fi SSID and password (2.4 GHz network or iPhone hotspot with "Maximize Compatibility" ON)
   - Firebase host + database secret
   - Telegram bot token and chat ID (create via [@BotFather](https://t.me/BotFather))
   - Gemini API key — free at https://aistudio.google.com/apikey (no billing required; 15 req/min / 1,500 req/day)
4. Open `source/main/main.ino` → Board = `ESP32 Dev Module` → Upload.
5. Open Serial Monitor at **9600 baud** to watch `[WiFi]`, `[Firebase]`, `[Telegram]`, `[Gemini]` logs.

## Cloud Monitoring

| Service | Where to watch |
|---|---|
| Firebase RTDB | Firebase Console → Realtime Database → Data — `/latest` refreshes every 1 s |
| Telegram | Chat with the bot; `/status` round-trips in ~3 s |
| Gemini | https://aistudio.google.com/apikey → click key → request count + quota |

## Workload

| Member | Board | Responsibilities |
|---|---|---|
| Member 1 | MCXC444 | HC-SR04, Photoresistor, Sound ISR, vDistanceTask, vEnvTask |
| Member 2 | MCXC444 | vAlertTask, vCommsTask, RGB LED, UART, RTOS mutexes |
| Member 3 | ESP32 | DHT11, LCD Display (I2C), Active Buzzer, UART parsing |
| Member 4 | ESP32 | Firebase RTDB, Telegram Bot API, Gemini API, Wi-Fi/HTTP |

## Project Structure

```
source/
  main/                      ESP32 Arduino sketch
    main.ino                 Entry: setup() + loop() + cloud orchestration
    config.h                 DeskState struct, pin map, thresholds
    secrets.h.example        Template for credentials (copy to secrets.h)
    wifi_manager.cpp/.h      Wi-Fi connect + auto-reconnect
    firebase_client.cpp/.h   Firebase RTDB logging via HTTPS REST
    telegram_bot.cpp/.h      Telegram getUpdates + sendMessage + command parser
    gemini_client.cpp/.h     Gemini generateContent call + response parse
    uart_receive.cpp/.h      Parse $MCXC frames, send $ESP frames
    sensors.cpp/.h           DHT11, HC-SR04, threshold evaluation
    display.cpp/.h           LCD 16x2 output
    comms.cpp/.h             Shared UART helpers
  (MCXC444 FreeRTOS application — sibling folder)
```
