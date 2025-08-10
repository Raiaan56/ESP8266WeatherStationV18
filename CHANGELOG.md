# Weather Station — Version History

## V18 — Released [2025-08-09]
- 🔧 Naming Cleanup:
  - Renamed WARM_LED → AVERAGE_LED (code + web JSON).
  - Renamed warmPWM → avgPWM for clarity.
- 🖥 Theme & Identity:
  - Removed alternate LCD theme; Retro CRT now default and only.
  - Updated web page title to “Weather Station V18”.
  - mDNS TXT record version bumped to "18".
- 🔌 Web UI Controls:
  - Added toolbar buttons:
    - Autoscroll toggle.
    - Timestamp toggle (client-side).
    - Clear Output (server-side `/logs/clear`).
- 📱 Mobile Optimizations:
  - Sticky toolbar stabilized with correct z-index and background.
- 💡 LED Animation:
  - Breathing speed slowed: phase increment 0.01.
- 📊 API:
  - JSON `/api` endpoint returns updated LED object `{ hot, average, cold }`.

## V16.x — Experimental Builds
- Minor UI tweaks and bug regressions.
- Theme toggle refined.
- Average temperature band added (renamed in UI only).

## V16 — Stable Baseline
- 🕒 Resilient Timekeeping:
  - NTP sync every 60 s (UTC).
  - Local time with DST using Timezone library.
  - Offline time maintained via epoch + millis().
- 📺 OLED Output:
  - Lines:
    - Temp / Humidity
    - SSID / IP / Hostname.local (rotates every 3 s)
    - Time ↔ Date (toggles every 3 s)
- 🌐 Web UI:
  - Retro theme with optional LCD mode.
  - Panels: Live Data, Serial Log, History Table.
  - Autorefresh of data/logs every 2 s.
- 🧠 History:
  - 600-entry ring buffer for Temp/Humidity.
  - Endpoints: `/history.json` and `/history.csv`.
- 🔥 LEDs:
  - HOT (≥ 76°F), AVERAGE (66–75°F), COLD (< 66°F).
  - Breathing effect; phase increment 0.02.
- 🔎 Logging:
  - Rolling 30-line buffer, prefixed with `<millis> ms: …`.

## Pre-V16 — Early Builds
- Basic ESP8266 setup with SSD1306 OLED, DHT22, and 3 LEDs.
- Simple web server on port 80.
- Button mode cycling and direct DHT sensor reads.
