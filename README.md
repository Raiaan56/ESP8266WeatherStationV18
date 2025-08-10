# ESP8266 Weather Station V18

A retro-themed, Wi‑Fi–enabled weather station powered by ESP8266. It reads temperature and humidity via a DHT22 sensor, displays info on an SSD1306 OLED, and shows temperature bands through animated LED breathing. Fully controllable via a web-based CRT-style UI with serial logging and history tracking.

## 📖 Table of Contents
- [Features](#-features)
- [Getting Started](#-getting-started)
- [Wiring Summary](#-wiring-summary)
- [Flashing the Code](#-flashing-the-code)
- [Accessing the UI](#-accessing-the-ui)
- [API Endpoints](#-api-endpoints)
- [Future Improvements](#-future-improvements)
- [License & Credits](#-license--credits)
- [UI Preview](assets/ui-preview.png)


## 🌟 Features
- NTP-based resilient time/date handling with offline fallback
- Web server with retro CRT UI theme (mobile-friendly)
- OLED display with rotating info & time/date toggle
- Three animated LEDs showing temp bands: HOT / AVERAGE / COLD
- Serial logging with autoscroll, timestamp toggle, and clear output
- Mode cycling: LIGHTS / SCREEN / BOTH / OFF
- Web API for sensor data, logs, device info, and history export

## 🚀 Getting Started

### 🧰 Hardware
- ESP8266 (NodeMCU or Wemos)
- DHT22 sensor
- SSD1306 OLED (128×32, I2C)
- 3 LEDs + 1 button
- Breadboard, jumper wires, or soldered board

### 🔌 Wiring Summary
| Component | GPIO Pin | Notes |
|-----------|----------|-------|
| DHT22     | D4       | Temp/Humidity |
| OLED (I2C)| SDA/SCL  | Default I2C 0x3C |
| HOT LED   | D5       | Red |
| AVERAGE LED | D6     | Yellow/White |
| COLD LED  | D7       | Blue |
| Button    | D3       | INPUT_PULLUP |

### 📦 Flashing the Code
1. Clone this repo.
2. Install dependencies:
   - Arduino core for ESP8266
   - TimeLib, Timezone, Adafruit GFX/SSD1306, ESPAsyncWebServer
3. Upload `weather_station_v18.ino` to your ESP8266 board.

### 🌐 Accessing the UI
- Connect to the same Wi‑Fi network.
- Visit `http://HOSTNAME.local` or IP in your browser.

## 📜 API Endpoints
- `/api` — Device info and sensor data
- `/logs` — Serial output (last 30 lines)
- `/toggleMode` — Cycle LIGHTS/SCREEN/BOTH/OFF
- `/refresh` — Force sensor read + NTP sync
- `/history.json` — 600-sample data history
- `/history.csv` — CSV export of data history

## 📷 Screenshots
Coming soon—UI preview and LED behavior demo.

## 📁 Files
- `weather_station_v18.ino` — Main firmware
- `CHANGELOG.md` — Version history
- `PRODUCT_SPEC.md` — Hardware and feature breakdown
- `README.md` — This document

## 🔮 Future Improvements
- Sensor smoothing (EMA)
- Persistent settings via EEPROM
- Longer-term logging (SD or cloud)
- Manual time/date fallback

## 🧠 License & Credits
Open Source License. Built with love by Cody. Retro theme inspired by CRT displays and 90s PC vibes.

