/****************************************************
  ESP8266 | DHT22 | OLED | 3×LEDs | Button
  Retro Weather Station + DST-Aware Clock  (V18)

  HOT     (≥ 76 °F) -> D5
  AVERAGE (66–75 °F) -> D6
  COLD    (< 66 °F) -> D7
****************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266NetBIOS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <Timezone.h>

/* ---------- Wi-Fi ---------- */
const char* WIFI_SSID = "RRAD13-2.4G";
const char* WIFI_PASS = "1131992r";
const uint16_t HTTP_PORT = 80;
const char* HOSTNAME = "weatherstation";  // access via http://weatherstation.local
ESP8266WebServer server(HTTP_PORT);

/* ---------- Time (NTP + offline keepalive) ---------- */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60 * 1000);  // UTC, update every 60s

TimeChangeRule CST = { "CST", First, Sun, Nov, 2, -360 };
TimeChangeRule CDT = { "CDT", Second, Sun, Mar, 2, -300 };
Timezone Central(CDT, CST);

// Keep last good epoch and advance it locally when offline
time_t lastGoodEpoch = 0;
uint32_t lastEpochMillis = 0;
bool timeSynced = false;

/* ---------- Pins ---------- */
#define DHTPIN D4
#define DHTTYPE DHT22
#define OLED_ADDR 0x3C
#define SCREEN_W 128
#define SCREEN_H 32
#define HOT_LED D5
#define AVERAGE_LED D6
#define COLD_LED D7
#define BTN_PIN D3

/* ---------- Devices ---------- */
Adafruit_SSD1306 oled(SCREEN_W, SCREEN_H, &Wire, -1);
DHT dht(DHTPIN, DHTTYPE);

/* ---------- State ---------- */
enum Mode { LIGHTS = 0, SCREEN, BOTH, OFF };
Mode mode = BOTH;
Mode prevMode = OFF;

bool btnPrev = HIGH;
float tC = NAN, h = NAN;
float last_tC = NAN, last_h = NAN;
float phase = 0.0;  // global phase for breathing animation
uint8_t infoIndex = 0;    // 3rd line: 0=SSID, 1=IP:port, 2=hostname
bool showTimeLine = true; // 4th line toggles Time/Date
uint32_t lastDeb = 0;
uint32_t lastInfoToggle = 0; // unified toggle timer (both lines)
uint32_t lastRead = 0;
const uint32_t DB_MS = 150;
const uint32_t INFO_TOGGLE_MS = 3000; // 3 seconds for both lines

// Track LED PWM states for web reporting
uint8_t hotPWM = 0, avgPWM = 0, coldPWM = 0;

/* ---------- Rolling serial log for web ---------- */
const int LOG_LINES = 30;
String logBuf[LOG_LINES];
int logHead = 0;
int logCount = 0;

/* ---------- Sensor history (timestamped) ---------- */
// Store last 600 samples (~20 minutes at 2s/sample)
const int HISTORY_SIZE = 600;
float histT[HISTORY_SIZE];
float histH[HISTORY_SIZE];
time_t histEpoch[HISTORY_SIZE]; // UTC epoch per sample
int histHead = 0;
int histCount = 0;

void logLine(const String& s) {
  Serial.println(s);
  String line = String(millis()) + " ms: " + s;
  logBuf[logHead] = line;
  logHead = (logHead + 1) % LOG_LINES;
  if (logCount < LOG_LINES) logCount++;
}

void pushHistory(float tc, float hum, time_t epochUTC) {
  if (isnan(tc) || isnan(hum) || epochUTC == 0) return; // only store valid, time-stamped data
  histT[histHead] = tc;
  histH[histHead] = hum;
  histEpoch[histHead] = epochUTC;
  histHead = (histHead + 1) % HISTORY_SIZE;
  if (histCount < HISTORY_SIZE) histCount++;
}

/* ---------- Helpers ---------- */
void setPWM(uint8_t pin, uint8_t value) {
  analogWrite(pin, value);
  if (pin == HOT_LED) hotPWM = value;
  else if (pin == AVERAGE_LED) avgPWM = value;
  else if (pin == COLD_LED) coldPWM = value;
}

// Slower breathing: quarter speed of original (V16 was 0.02 from 0.04); now 0.01
void breathe(uint8_t pin) {
  phase += 0.01f; // slower breathing
  if (phase > TWO_PI) phase = 0;
  uint8_t v = uint8_t((sin(phase) * 0.5f + 0.5f) * 255);
  setPWM(pin, v);
}

void ledsOff() {
  setPWM(HOT_LED, 0);
  setPWM(AVERAGE_LED, 0);
  setPWM(COLD_LED, 0);
}

void drawWaiting() {
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("Waiting for sensor...");
  oled.display();
}

String wifiStrengthLabel(int rssi) {
  if (WiFi.status() != WL_CONNECTED) return "No Link";
  if (rssi >= -60) return "Strong";
  if (rssi >= -75) return "Average";
  return "Weak";
}

time_t currentEpochUTC() {
  // If we've ever synced, advance from lastGoodEpoch using millis
  if (lastGoodEpoch > 0) {
    uint32_t delta = (millis() - lastEpochMillis) / 1000;
    return lastGoodEpoch + delta;
  }
  // Never synced yet
  return 0;
}

String formatTime12(time_t local) {
  int hr = hour(local);
  int mm = minute(local);
  int ss = second(local);
  int hr12 = hr % 12;
  if (hr12 == 0) hr12 = 12;
  const char* meridiem = (hr < 12) ? "AM" : "PM";
  char buf[16];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d %s", hr12, mm, ss, meridiem);
  return String(buf);
}

String formatDateMDY(time_t local) {
  int M = month(local);
  int D = day(local);
  int Y = year(local);
  char buf[16];
  snprintf(buf, sizeof(buf), "%02d/%02d/%04d", M, D, Y);
  return String(buf);
}

void drawData() {
  oled.clearDisplay();
  oled.setCursor(0, 0);
  float tF = tC * 1.8 + 32.0;
  oled.printf("Temp: %.1fC / %.1fF", tC, tF);

  oled.setCursor(0, 8);
  oled.printf("Humidity: %.1f%%", h);

  // 3rd line: SSID / IP:PORT / HOSTNAME.local (sync’d with line 4)
  oled.setCursor(0, 16);
  if (WiFi.status() == WL_CONNECTED) {
    if (infoIndex == 0) {
      oled.printf("SSID: %s *", WIFI_SSID);
    } else if (infoIndex == 1) {
      IPAddress ip = WiFi.localIP();
      oled.printf("%s:%u", ip.toString().c_str(), HTTP_PORT);
    } else {
      oled.print(String(HOSTNAME) + ".local");
    }
  } else {
    oled.print("No Connection");
  }

  // 4th line: toggle time/date every 3s, keep last value if offline
  oled.setCursor(0, 24);
  time_t nowUTC = currentEpochUTC();
  if (nowUTC > 0) {
    time_t local = Central.toLocal(nowUTC);
    if (showTimeLine) oled.print("Time: " + formatTime12(local));
    else oled.print("Date: " + formatDateMDY(local));
  } else {
    oled.print("Date: --/--/----");
  }

  oled.display();
}

/* ---------- Web UI (mobile-first + retro theme) ---------- */
String htmlPage() {
  IPAddress ip = WiFi.localIP();

  String html =
    "<!DOCTYPE html><html><head><meta charset='utf-8'/>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'/>"
    "<title>Weather Station V18</title>"
    "<style>"
    ":root{--bg:#000;--fg:#00ff6a;--accent:#00ff6a;--label:#00ffaa;--value:#fff;--frameBorder:#00ff6a;--panelBorder:#00ff6a;--preBg:#010;--preBorder:#0f5}"
    "body{background:var(--bg);color:var(--fg);font-family:Consolas,monospace;margin:0}"
    ".crt{position:relative;min-height:100vh;padding:16px}"
    ".frame{border:4px solid var(--frameBorder);padding:12px;box-shadow:0 0 18px var(--frameBorder) inset}"
    "h1{margin:0 0 12px 0;text-transform:uppercase;letter-spacing:2px}"
    ".toolbar{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:8px}"
    ".grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}"
    ".panel{border:2px dashed var(--panelBorder);padding:8px}"
    ".label{color:var(--label)}"
    ".value{color:var(--value)}"
    "button{background:#111;color:var(--fg);border:2px solid var(--accent);padding:10px 14px;cursor:pointer;min-height:44px;border-radius:6px}"
    "button:hover{background:var(--accent);color:#000}"
    "pre{white-space:pre-wrap;max-height:220px;overflow:auto;background:var(--preBg);border:1px solid var(--preBorder);padding:8px}"
    ".leds span{display:inline-block;margin-right:12px}"
    ".weak{color:#ff5b5b}.avg{color:#ffd65b}.strong{color:#7bff5b}"
    ".footer{margin-top:10px;font-size:12px;color:#0f5}"
    "table{width:100%;border-collapse:collapse}"
    "td,th{border:1px solid #0f5;padding:4px}"
    "@media (max-width:600px){"
      ".grid{grid-template-columns:1fr}"
      ".frame{box-shadow:none}"
      ".panel{border-style:solid}"
      /* Mobile toolbar stability fix: solid bg + z-index + sticky */
      ".toolbar{position:sticky;top:0;background:var(--bg);z-index:100;padding-bottom:8px}"
    "}"
    ".offline{position:fixed;left:12px;right:12px;bottom:12px;padding:10px 12px;border-radius:6px;background:#2b1a1a;color:#ffcccc;border:1px solid #6d2b2b;box-shadow:0 6px 24px rgba(0,0,0,.35);display:none}"
    "</style>"
    "</head><body><div class='crt'><div class='frame'>"
    "<h1>Weather Station V18</h1>"

    "<div class='toolbar'>"
      "<button onclick='doRefresh()'>Refresh Data</button>"
      "<button onclick='doToggleMode()'>Toggle Mode</button>"
      "<button id='autoBtn' onclick='toggleAutoscroll()'>Autoscroll: On</button>"
      "<button id='tsBtn' onclick='toggleTimestamps()'>Timestamps: On</button>"
      "<button onclick='clearLogs()'>Clear Output</button>"
    "</div>"

    "<div class='grid'>"
      "<div class='panel'>"
        "<div><span class='label'>Temp:</span> <span id='temp' class='value'>--</span></div>"
        "<div><span class='label'>Humidity:</span> <span id='hum' class='value'>--</span></div>"
        "<div><span class='label'>SSID:</span> <span id='ssid' class='value'>--</span></div>"
        "<div><span class='label'>IP:</span> <span id='ip' class='value'>--</span></div>"
        "<div><span class='label'>Hostname:</span> <span id='host' class='value'>--</span></div>"
        "<div><span class='label'>Access:</span> <span class='value'>http://" + String(HOSTNAME) + ".local</span></div>"
        "<div><span class='label'>Time:</span> <span id='time' class='value'>--</span></div>"
        "<div><span class='label'>Date:</span> <span id='date' class='value'>--</span></div>"
        "<div><span class='label'>RSSI:</span> <span id='rssi' class='value'>--</span>"
        " (<span id='strength' class='value'>--</span>)</div>"
        "<div><span class='label'>Mode:</span> <span id='mode' class='value'>--</span></div>"
        "<div class='leds'><span class='label'>LEDs:</span>"
          "<span>HOT: <b id='hot'>OFF</b></span>"
          "<span>AVERAGE: <b id='average'>OFF</b></span>"
          "<span>COLD: <b id='cold'>OFF</b></span>"
        "</div>"
      "</div>"
      "<div class='panel'>"
        "<div class='label'>Serial Output</div>"
        "<pre id='logs'>(waiting...)</pre>"
      "</div>"
    "</div>"

    "<div class='panel' style='margin-top:12px'>"
      "<div class='label'>Temperature/Humidity History (latest first)</div>"
      "<div style='margin:6px 0'>"
        "<button onclick='fetchHistory()'>Refresh History</button> "
        "<a href=\"/history.csv\" target=\"_blank\"><button>Download CSV</button></a>"
      "</div>"
      "<table>"
        "<thead><tr><th>Time</th><th>Date</th><th>Temp (C/F)</th><th>Humidity (%)</th></tr></thead>"
        "<tbody id='hist'></tbody>"
      "</table>"
    "</div>"
    "<div class='footer'>Auto-refresh data/logs every 2s.</div>"
    "</div></div>"

    "<div id='offline' class='offline'>Offline — reconnecting…</div>"

    "<script>"
    "let autoScroll=true, showTS=true;"

    "function cls(n){return n>=-60?'strong':(n>=-75?'avg':'weak');}"
    "function fetchAPI(){fetch('/api').then(r=>r.json()).then(j=>{"
      "document.getElementById('temp').textContent=j.tempC.toFixed(1)+' °C / '+j.tempF.toFixed(1)+' °F';"
      "document.getElementById('hum').textContent=j.humidity.toFixed(1)+' %';"
      "document.getElementById('ssid').textContent=j.ssid;"
      "document.getElementById('ip').textContent=j.ip;"
      "document.getElementById('host').textContent=j.hostname;"
      "document.getElementById('time').textContent=j.time;"
      "document.getElementById('date').textContent=j.date;"
      "document.getElementById('rssi').textContent=j.rssi+' dBm';"
      "let s=document.getElementById('strength'); s.textContent=j.strength; s.className=cls(j.rssi);"
      "document.getElementById('mode').textContent=j.mode;"
      "document.getElementById('hot').textContent=j.leds.hot?'ON':'OFF';"
      "document.getElementById('average').textContent=j.leds.average?'ON':'OFF';"
      "document.getElementById('cold').textContent=j.leds.cold?'ON':'OFF';"
    "}).catch(_=>{});} "

    "function transformLogs(t){"
      "if(showTS) return t;"
      "return t.split('\\n').map(line=>line.replace(/^\\s*\\d+\\s*ms:\\s*/,'')).join('\\n');"
    "}"
    "function fetchLogs(){fetch('/logs').then(r=>r.text()).then(t=>{"
      "const pre=document.getElementById('logs');"
      "pre.textContent=transformLogs(t);"
      "if(autoScroll){pre.scrollTop=pre.scrollHeight;}"
    "}).catch(_=>{});} "

    "function fetchHistory(){fetch('/history.json').then(r=>r.json()).then(arr=>{"
      "let tbody=document.getElementById('hist');"
      "tbody.innerHTML='';"
      "arr.forEach(e=>{"
        "let tr=document.createElement('tr');"
        "tr.innerHTML='<td>'+e.time+'</td><td>'+e.date+'</td><td>'+e.tempC.toFixed(1)+' / '+e.tempF.toFixed(1)+'</td><td>'+e.humidity.toFixed(1)+'</td>';"
        "tbody.appendChild(tr);"
      "});"
    "}).catch(_=>{});} "

    "function doRefresh(){fetch('/refresh').then(_=>{fetchAPI();fetchLogs();fetchHistory();});} "
    "function doToggleMode(){fetch('/toggleMode').then(_=>{fetchAPI();});} "

    "function toggleAutoscroll(){autoScroll=!autoScroll; document.getElementById('autoBtn').textContent='Autoscroll: '+(autoScroll?'On':'Off');}"
    "function toggleTimestamps(){showTS=!showTS; document.getElementById('tsBtn').textContent='Timestamps: '+(showTS?'On':'Off'); fetchLogs();}"
    "function clearLogs(){fetch('/logs/clear').then(_=>{document.getElementById('logs').textContent='';});}"

    "setInterval(fetchAPI,2000); setInterval(fetchLogs,2000);"
    "window.onload=()=>{fetchAPI();fetchLogs();fetchHistory();startPing();};"

    "function setOffline(x){document.getElementById('offline').style.display=x?'block':'none';}"
    "async function ping(){try{const r=await fetch('/api/ping',{cache:'no-store'}); setOffline(!r.ok);}catch(e){setOffline(true);}}"
    "function startPing(){ping(); setInterval(ping,5000);} "
    "window.addEventListener('online', ()=>ping());"
    "window.addEventListener('offline', ()=>setOffline(true));"
    "</script>"
    "</body></html>";
  return html;
}

String modeName(Mode m) {
  switch (m) {
    case LIGHTS: return "LIGHTS";
    case SCREEN: return "SCREEN";
    case BOTH:   return "BOTH";
    case OFF:    return "OFF";
  }
  return "UNKNOWN";
}

/* ---------- Network Boot Summary ---------- */
void bootNetworkStatus() {
  unsigned long wifiStart = millis();
  const unsigned long wifiTimeout = 10000;
  bool wifiOK = false;

  // Wait up to wifiTimeout for WiFi
  while (millis() - wifiStart < wifiTimeout) {
    if (WiFi.status() == WL_CONNECTED) { wifiOK = true; break; }
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  oled.setCursor(0, 8); // OLED line 2
  bool nbnsOK = false, mdnsOK = false;

  if (wifiOK) {
    logLine("WiFi connected. IP: " + WiFi.localIP().toString());
    // Start NetBIOS and mDNS
    nbnsOK = NBNS.begin(HOSTNAME);
    if (!nbnsOK) logLine("NBNS failed to start."); else logLine(String("NBNS started: ") + HOSTNAME);

    mdnsOK = MDNS.begin(HOSTNAME);
    if (mdnsOK) {
      MDNS.addService("http", "tcp", HTTP_PORT);
      MDNS.addServiceTxt("http", "tcp", "path", "/");
      MDNS.addServiceTxt("http", "tcp", "board", "esp8266");
      MDNS.addServiceTxt("http", "tcp", "ver", "18");
      logLine(String("mDNS: http://") + HOSTNAME + ".local");
      logLine("mDNS service: _http._tcp advertised");
    } else {
      logLine("mDNS failed to start.");
    }

    oled.print("Net OK  | ");
    oled.print(mdnsOK ? "DNS OK" : "DNS FAIL");
  } else {
    logLine("WiFi failed. Offline mode.");
    oled.print("Net FAIL | DNS OFF");
  }

  oled.display();
}

/* ---------- Setup ---------- */
void setup() {
  Serial.begin(115200);
  analogWriteRange(255);
  pinMode(HOT_LED, OUTPUT);
  pinMode(AVERAGE_LED, OUTPUT);
  pinMode(COLD_LED, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED FAIL");
    while (true) yield();
  }
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  drawWaiting();

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);        // set DHCP hostname
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  logLine(String("Connecting to ") + WIFI_SSID);

  // Accurate boot status after real WiFi attempt
  bootNetworkStatus();

  // NTP
  timeClient.begin();
  if (timeClient.update()) {
    lastGoodEpoch = timeClient.getEpochTime();
    lastEpochMillis = millis();
    timeSynced = true;
    logLine("Time synced via NTP.");
  } else {
    logLine("Initial NTP sync failed; will retry.");
  }

  // Web routes
  server.on("/", [=]() {
    server.send(200, "text/html", htmlPage());
  });

  server.on("/api", [=]() {
    float tF = tC * 1.8 + 32.0;
    String ssidStr = (WiFi.status() == WL_CONNECTED) ? String(WIFI_SSID) : String("(offline)");
    String ipStr = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : String("(IP unset)");
    int rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : -1000;
    String strength = wifiStrengthLabel(rssi);

    time_t nowUTC = currentEpochUTC();
    String timeStr = "--:--:-- --";
    String dateStr = "--/--/----";
    if (nowUTC > 0) {
      time_t local = Central.toLocal(nowUTC);
      timeStr = formatTime12(local);
      dateStr = formatDateMDY(local);
    }

    String json = "{";
    json += "\"tempC\":" + String(isnan(tC) ? 0.0f : tC, 1) + ",";
    json += "\"tempF\":" + String(isnan(tC) ? 0.0f : (tC * 1.8 + 32.0), 1) + ",";
    json += "\"humidity\":" + String(isnan(h) ? 0.0f : h, 1) + ",";
    json += "\"ssid\":\"" + ssidStr + "\",";
    json += "\"ip\":\"" + ipStr + "\",";
    json += "\"hostname\":\"" + String(HOSTNAME) + ".local\",";
    json += "\"time\":\"" + timeStr + "\",";
    json += "\"date\":\"" + dateStr + "\",";
    json += "\"rssi\":" + String(rssi) + ",";
    json += "\"strength\":\"" + strength + "\",";
    json += "\"mode\":\"" + modeName(mode) + "\",";
    json += "\"leds\":{";
    json += "\"hot\":" + String(hotPWM > 0 ? "true" : "false") + ",";
    json += "\"average\":" + String(avgPWM > 0 ? "true" : "false") + ",";
    json += "\"cold\":" + String(coldPWM > 0 ? "true" : "false");
    json += "}";
    json += "}";

    server.send(200, "application/json", json);
  });

  // Health ping for offline overlay
  server.on("/api/ping", [=]() {
    server.send(200, "text/plain", "ok");
  });

  server.on("/logs", [=]() {
    String out;
    for (int i = 0; i < logCount; i++) {
      int idx = (logHead - logCount + i + LOG_LINES) % LOG_LINES;
      out += logBuf[idx] + "\n";
    }
    server.send(200, "text/plain", out);
  });

  // Clear logs buffer
  server.on("/logs/clear", [=]() {
    for (int i = 0; i < LOG_LINES; i++) logBuf[i] = "";
    logHead = 0;
    logCount = 0;
    server.send(200, "text/plain", "cleared");
  });

  // Toggle like the physical button on D3
  server.on("/toggleMode", [=]() {
    mode = Mode((mode + 1) % 4);
    logLine("Mode changed to " + modeName(mode) + " (via web)");
    server.send(200, "text/plain", "OK");
  });

  // Force refresh
  server.on("/refresh", [=]() {
    lastRead = 0; // force sensor read soon
    if (timeClient.update()) {
      lastGoodEpoch = timeClient.getEpochTime();
      lastEpochMillis = millis();
      timeSynced = true;
      logLine("Time refreshed via NTP.");
    }
    server.send(200, "text/plain", "OK");
  });

  // History endpoints
  server.on("/history.json", [=]() {
    String out = "[";
    for (int i = 0; i < histCount; i++) {
      int idx = (histHead - 1 - i + HISTORY_SIZE) % HISTORY_SIZE; // latest first
      time_t utc = histEpoch[idx];
      time_t loc = Central.toLocal(utc);
      String tStr = formatTime12(loc);
      String dStr = formatDateMDY(loc);
      float tc = histT[idx];
      float tf = tc * 1.8 + 32.0;
      float hh = histH[idx];

      out += "{\"time\":\"" + tStr + "\",\"date\":\"" + dStr + "\",";
      out += "\"tempC\":" + String(tc, 1) + ",\"tempF\":" + String(tf, 1) + ",";
      out += "\"humidity\":" + String(hh, 1) + "}";
      if (i != histCount - 1) out += ",";
    }
    out += "]";
    server.send(200, "application/json", out);
  });

  server.on("/history.csv", [=]() {
    String out = "epochUTC,local_time,local_date,tempC,tempF,humidity\n";
    for (int i = 0; i < histCount; i++) {
      int idx = (histHead - 1 - i + HISTORY_SIZE) % HISTORY_SIZE; // latest first
      time_t utc = histEpoch[idx];
      time_t loc = Central.toLocal(utc);
      String tStr = formatTime12(loc);
      String dStr = formatDateMDY(loc);
      float tc = histT[idx];
      float tf = tc * 1.8 + 32.0;
      float hh = histH[idx];
      out += String((unsigned long)utc) + "," + tStr + "," + dStr + ",";
      out += String(tc, 1) + "," + String(tf, 1) + "," + String(hh, 1) + "\n";
    }
    server.send(200, "text/csv", out);
  });

  server.begin();
  logLine("Web server started.");
}

/* ---------- Loop ---------- */
void loop() {
  server.handleClient();
  MDNS.update();

  // Button (mode cycle)
  bool btnNow = digitalRead(BTN_PIN);
  if (btnPrev == HIGH && btnNow == LOW && millis() - lastDeb > DB_MS) {
    mode = Mode((mode + 1) % 4);
    lastDeb = millis();
    logLine("Mode changed to " + modeName(mode) + " (via button)");
  }
  btnPrev = btnNow;

  // Sensor read every 2s
  if (millis() - lastRead > 2000) {
    lastRead = millis();
    tC = dht.readTemperature();
    h = dht.readHumidity();
    time_t nowUTC = currentEpochUTC();
    if (!isnan(tC) && !isnan(h)) {
      pushHistory(tC, h, nowUTC);
      logLine("Sensor: " + String(tC, 1) + "C, " + String(tC * 1.8 + 32.0, 1) + "F, " + String(h, 1) + "%");
    }
  }

  // Lights
  if (mode == LIGHTS || mode == BOTH) {
    ledsOff();
    if (!isnan(tC)) {
      float tF = tC * 1.8 + 32.0;
      if (tF >= 76)      breathe(HOT_LED);
      else if (tF >= 66) breathe(AVERAGE_LED);  // Labeled "AVERAGE" on web UI
      else               breathe(COLD_LED);
    }
  } else {
    ledsOff();
  }

  // Unified toggle: every 3s advance both the infoIndex (SSID/IP/Host) and the time/date line
  if (millis() - lastInfoToggle > INFO_TOGGLE_MS) {
    infoIndex = (infoIndex + 1) % 3;
    showTimeLine = !showTimeLine;
    lastInfoToggle = millis();
  }

  // NTP keepalive: update; on success, refresh lastGoodEpoch
  if (timeClient.update()) {
    lastGoodEpoch = timeClient.getEpochTime();
    lastEpochMillis = millis();
    if (!timeSynced) logLine("Time synced via NTP.");
    timeSynced = true;
  }

  // Update screen if needed
  bool dataChanged = (tC != last_tC) || (h != last_h);
  bool modeChanged = (mode != prevMode);
  bool toggleChange = (millis() - lastInfoToggle < 50);
  prevMode = mode;

  if (mode == SCREEN || mode == BOTH) {
    if (dataChanged || modeChanged || toggleChange) {
      if (isnan(tC) || isnan(h)) drawWaiting();
      else drawData();
      last_tC = tC; last_h = h;
    }
  } else if (modeChanged) {
    oled.clearDisplay();
    oled.display();
  }

  delay(10);
}