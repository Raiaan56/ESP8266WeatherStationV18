# Weather Station V18 — Architecture Overview

## 🧩 Subsystems

### 1. Sensor Acquisition
- DHT22 read every 2000 ms
- Data validated before use
- Values pushed to RAM ring buffer (600 entries)

### 2. Display Logic
- OLED draws Temp/Humidity + network info
- SSID/IP/Hostname + Time/Date cycle every 3 s

### 3. LED Control
- Determine band based on °F
- Animate only the matching LED using sine-based PWM
- Phase increments at 0.01f per loop for breathing effect

### 4. Time System
- NTP sync every 60 s to UTC
- Timezone library converts to CDT/CST
- Local time fallback via millis() if NTP unreachable

### 5. Web Interface
- Served from ESP8266 at `/`
- Panels: Sensor, Serial Output, History
- Toolbar: Action buttons with mode toggle, timestamp toggle, autoscroll

### 6. API and Endpoints
| Endpoint       | Function                  |
|----------------|---------------------------|
| `/api`         | Sensor + device info      |
| `/logs`        | 30-line buffer            |
| `/toggleMode`  | Cycle device mode         |
| `/refresh`     | Refresh sensor/NTP        |
| `/history.json`| Data history              |
| `/history.csv` | Export CSV                |
| `/logs/clear`  | Clear log buffer          |
| `/api/ping`    | Health check              |

## 🧠 Data Flow
- Sensor → RAM Buffer → OLED + Web
- Serial Log → Web Log Viewer
- Mode state: Button ⇄ Web API

## 🔐 Security Notes
- No password protection (LAN-only access recommended)
- API calls via browser only
- Future candidate: admin token for writes

