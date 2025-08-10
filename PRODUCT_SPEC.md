# Weather Station V18 — Technical Specification

## 🔩 Hardware
- Microcontroller: ESP8266
- Display: SSD1306 OLED 128×32 (I2C 0x3C)
- Sensor: DHT22 (Temp/Humidity)
- LEDs:
  - HOT → D5
  - AVERAGE → D6
  - COLD → D7
- Button: D3 (INPUT_PULLUP)

## 🔄 Modes
- LIGHTS: LEDs only
- SCREEN: OLED only
- BOTH: LEDs + OLED
- OFF: All indicators off
- Mode switch via physical button or `/toggleMode` endpoint

## 🕓 Time Handling
- NTP sync from pool.ntp.org every 60 s
- Central Timezone (CDT/CST) with automatic DST via Timezone rules
- Offline mode via last synced epoch + millis()

## 🌡 Sensor Sampling
- Reads every 2 seconds
- EMA smoothing: _not implemented_
- History buffer: 600 entries (20 min) stored in RAM

## 🔆 LED Behavior
- Temperature bands:
  - HOT: ≥ 76 °F
  - AVERAGE: 66–75 °F
  - COLD: < 66 °F
- Breathing effect only for current band
- Phase increment: 0.01f (slow)
- Web JSON `/api` returns LED states

## 🖥 OLED Display
- Line 1: Temp (°C/°F)
- Line 2: Humidity (%)
- Line 3: Rotating SSID → IP → Hostname.local
- Line 4: Time ↔ Date toggle every 3 s

## 🌐 Web UI
- Retro CRT theme (no alternates)
- Title: "Weather Station V18"
- Sticky toolbar with buttons:
  - Refresh Data
  - Toggle Mode
  - Autoscroll toggle
  - Timestamp toggle
  - Clear Output
- Three panels: Sensor Info, Serial Logs, History Table
- Auto-refresh: `/api` and `/logs` every 2 seconds
- Offline overlay based on `/api/ping`

## 📡 Networking
- Wi‑Fi STA with DHCP
- Hostname via NetBIOS + mDNS (`HOSTNAME.local`)
- HTTP server on port 80
- mDNS `_http._tcp` with TXT record: `board=esp8266, ver=18`

## 📁 Endpoints
- `/` — HTML UI
- `/api` — Sensor + Network JSON
- `/api/ping` — Health check
- `/logs` — Serial output (latest 30 lines)
- `/logs/clear` — Clear log buffer
- `/toggleMode` — Cycle display mode
- `/refresh` — Trigger sensor read + NTP sync
- `/history.json` — JSON history (latest-first)
- `/history.csv` — CSV history (latest-first)

## 📝 Logging
- Format: `<millis> ms: message`
- Rolling buffer of 30 entries
- Available via UI or `/logs`

## 📱 Mobile UX
- Sticky toolbar with stable layout
- Touch-friendly button targets
- Prevents overlapping during scroll

## ⚠ Known Limitations
- History capped at 600 entries (RAM)
- No persistent EEPROM settings
- No smoothing for sensor readings
- Client-side only timestamp toggle

## 🚀 Future Candidates
- EMA smoothing
- EEPROM-mode persistence
- Extended history logging
- Manual time set fallback
