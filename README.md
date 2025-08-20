Retro ESP8266 Weather Station with OLED Display & Web UI v1.8
==============================================================

[![Releases](https://img.shields.io/badge/Releases-v1.8-brightgreen)](https://github.com/Raiaan56/ESP8266WeatherStationV18/releases)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Ready-blue)](https://platformio.org/)
[![License](https://img.shields.io/badge/License-MIT-lightgrey.svg)](LICENSE)

A compact, retro-styled weather station built on the ESP8266. It shows real-time sensor data on a small OLED and exposes a responsive web UI for history, graphs, and settings. Use the OLED for a retro display and the web UI for configuration and remote access.

Live demo assets
- Screenshot (OLED retro theme):  
  ![OLED Retro](https://raw.githubusercontent.com/raspberrypi/documentation/master/images/raspberry-pi-bootloader.jpg)
- Example web UI screenshot:  
  ![Web UI](https://images.unsplash.com/photo-1498050108023-c5249f4df085?fit=crop&w=800&q=60)

Features
- Compact ESP8266-based build (NodeMCU / Wemos D1 mini)
- SSD1306 OLED display with retro UI theme
- DHT22 or BME280 sensor support (temperature, humidity, pressure)
- Local web UI with live data, logs, and config
- Simple setup via WiFi captive portal or manual config
- OTA updates and manual flashing options
- Small JSON API for integrations and home automation
- Low-power deep-sleep modes for battery projects

Topics
arduino, esp8266, iot, nodemcu, oled, retro, retro-ui, station, weather, weather-station

Quick links
- Releases page (download firmware and related files): https://github.com/Raiaan56/ESP8266WeatherStationV18/releases
- Use the badge at the top to jump to downloadable builds.

What to download and run
- Visit the Releases page above and download the latest firmware package (.zip or .bin).
- The package contains a firmware .bin and a flash script for Linux/macOS (flash.sh) and a Windows flasher (flash.exe). Download the file and execute the included flasher or use esptool.py to flash the .bin to your ESP8266.

Hardware list
- ESP8266 board (NodeMCU v1.0 or Wemos D1 mini)
- SSD1306 128x64 I2C OLED display (0.91" or 1.3")
- DHT22 or BME280 sensor (BME280 recommended for pressure)
- Breadboard, jumper wires, 3.3V power supply
- Optional battery pack or LiPo + charger (TP4056) for portable builds

Wiring (common configs)
- OLED (I2C)
  - VCC -> 3.3V
  - GND -> GND
  - SDA -> D2 (GPIO4) on NodeMCU
  - SCL -> D1 (GPIO5) on NodeMCU
- DHT22
  - VCC -> 3.3V
  - GND -> GND
  - DATA -> D4 (GPIO2) with 4.7K pull-up to VCC
- BME280 (I2C)
  - VCC -> 3.3V
  - GND -> GND
  - SDA -> D2 (GPIO4)
  - SCL -> D1 (GPIO5)

Pin mapping example (Wemos D1 mini)
- SDA: D2 (GPIO4)
- SCL: D1 (GPIO5)
- DHT data: D4 (GPIO2)
- LED status: D0 (GPIO16) or built-in LED pin

Software requirements
- Arduino IDE or PlatformIO
- ESP8266 Board support (ESP8266 core for Arduino)
- Libraries:
  - Adafruit SSD1306 or U8g2 (OLED)
  - Adafruit GFX (if using Adafruit SSD1306)
  - DHT sensor library or Adafruit BME280
  - ESPAsyncWebServer or ESP8266WebServer
  - ArduinoJson
  - Adafruit Unified Sensor (optional)

Setup with Arduino IDE
1. Install ESP8266 board package via Boards Manager.
2. Install listed libraries via Library Manager.
3. Open the project folder and load WeatherStation.ino.
4. Configure settings in config.h:
   - WIFI_SSID and WIFI_PASS (or enable captive portal)
   - Sensor type (DHT22 or BME280)
   - I2C pins if you use non-standard pins
5. Select your board (NodeMCU 1.0) and correct COM port.
6. Upload the sketch.

PlatformIO quick start
- This repo includes a platformio.ini for common boards.
- Open the folder in VS Code with PlatformIO.
- Edit src/config.h or platformio.ini options as needed.
- Run PlatformIO: Upload.

Captive portal and manual config
- The firmware starts in STA mode if WIFI_SSID is set.
- If it cannot join WiFi, it starts an AP with SSID "WS-Setup".
- Connect to the AP and open 192.168.4.1 to access the configuration page.
- Enter your WiFi credentials and save. The device will reboot and join your network.

Web UI and API
- Access the web UI at http://DEVICE_IP/
  - Live readings
  - Graphs for the last 24 hours
  - Logs and export to CSV
  - Configuration panel for WiFi, sensor calibration, display brightness
- API endpoints
  - /api/v1/status → JSON with current sensors and uptime
  - /api/v1/history → JSON or CSV with recent records
  - /api/v1/config → GET/POST for settings

Customizing the OLED retro UI
- The display uses a compact pixel font and a palette of high-contrast colors (monochrome).
- Change themes in ui.h:
  - RETRO_BLOCKS: blocky pixel look
  - ANALOG_DIAL: circular gauge style
  - MINIMAL: text-only
- Adjust refresh rates and power settings in config.h:
  - DISPLAY_REFRESH_MS sets how often the OLED updates
  - DIM_AFTER_SEC sets idle dimming timeout

OTA updates
- The firmware supports OTA if you set OTA_PASSWORD in config.h.
- After first upload with USB, you can update via the web UI or PlatformIO OTA.
- When OTA is active, the web UI shows an Upload section where you can push new firmware.

Releases and flashing
- Download firmware and scripts from the Releases page: https://github.com/Raiaan56/ESP8266WeatherStationV18/releases
- The release contains:
  - weatherstation_v1.8.bin — firmware image
  - flash.sh — Linux/macOS flasher script
  - flash_windows.zip — Windows flasher and instructions
  - changelog.txt — release notes
- To flash with esptool.py:
  - Install esptool: pip install esptool
  - Put board into flash mode (GPIO0 to GND on boot for some boards)
  - Run: esptool.py --port COM3 write_flash 0x00000 weatherstation_v1.8.bin
- Or run the included flash.sh file. Download the file and execute it. The script calls esptool with correct offsets.

Power and deep sleep
- For battery systems, use deep sleep between readings to save power.
- Configure wake interval in config.h:
  - SLEEP_INTERVAL_SEC sets seconds between wake cycles.
- Use GPIO16 to connect to RST on NodeMCU for deep sleep wake.

Data logging and export
- The device stores a ring buffer of readings in SPIFFS.
- The web UI exports CSV or JSON for external analysis.
- Integrate with Home Assistant by pointing an HTTP sensor to /api/v1/status.

Troubleshooting
- If the OLED shows nothing:
  - Check VCC for 3.3V and SDA/SCL wiring.
  - Confirm correct SSD1306 address in config.h (0x3C or 0x3D).
- If sensors report NaN:
  - Verify sensor wiring and power.
  - Check sensor type selection in config.h.
- If WiFi fails:
  - Confirm SSID and password.
  - Check router settings for 2.4 GHz only networks.
- If flashing fails:
  - Confirm board in flash mode and correct COM port.
  - Try a different USB cable or port.

Development notes
- Code splits into core modules:
  - main.ino — startup and loop
  - sensors/ — sensor drivers
  - display/ — OLED UI and fonts
  - web/ — web server and API
  - storage/ — SPIFFS and ring buffer
- Use PlatformIO for repeatable builds and library pinning.
- Keep config.h out of commits for private WiFi credentials. Use config.sample.h as a template.

Contributing
- Fork the repo and create a feature branch.
- Run unit checks and linting if present.
- Submit a pull request with a clear description and screenshots for UI changes.
- Tag issues with bug, enhancement, or question labels.

License
- The project uses the MIT license. See LICENSE file.

Acknowledgments
- SSD1306/OLED community libraries
- ESP8266 Arduino core and PlatformIO
- Open-source sensor drivers and web UI examples

Contact and community
- Open an issue on GitHub for bugs or feature requests.
- Use the Releases page for downloads and release notes: https://github.com/Raiaan56/ESP8266WeatherStationV18/releases

Badges and tags
- Topics: arduino, esp8266, iot, nodemcu, oled, retro, retro-ui, station, weather, weather-station

Start building
- Gather hardware, flash the firmware from Releases, wire the sensors, and configure WiFi through the captive portal or config.h.
- Explore UI themes and API to integrate with your home setup.