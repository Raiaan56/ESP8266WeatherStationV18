# Weather Station v18 — Release Notes

Release date: 2025-08-09

## Highlights
- Single retro CRT theme; removed legacy LCD theme.
- Sticky toolbar fixes for a stable mobile experience.
- LED naming cleanup: WARM → AVERAGE (code + UI + API).
- Slower, smoother LED breathing (phase increment 0.01).
- Log viewer controls: Autoscroll, Timestamp toggle, Clear output.

## API
- `/api` now returns `leds: { hot, average, cold }`.
- `/logs/clear` clears rolling buffer.
- `/refresh` triggers near-term sensor read and NTP attempt.

## Hardware mapping
- DHT22: D4
- HOT LED: D5 (≥ 76 °F)
- AVERAGE LED: D6 (66–75 °F)
- COLD LED: D7 (< 66 °F)
- Button: D3 (mode cycle)

## Known limitations
- No EEPROM persistence for settings.
- No EMA smoothing; raw sensor values.
- History limited to ~20 minutes (600 samples).
- Timestamp toggle is client-side only.

## Upgrade notes
- If you relied on `warm` identifiers in client code, update to `average`.
- If you themed the UI via the old toggle, migrate styles to the CRT theme.
