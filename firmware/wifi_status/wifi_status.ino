#include <WiFi.h>
#include <WebServer.h>
#include <esp_timer.h>

#include "secrets.h"
#include "simulation.h"

WebServer server(80);
bool wasConnected = false;
uint32_t lastReconnectAt = 0;
Simulation simulation;
uint32_t lastSampleAt = 0;
String eventHistory = "";

const char* stateName() {
  switch (simulation.detector().state()) {
    case DetectorState::Calibrating: return "Calibrating";
    case DetectorState::Clear: return "No package";
    case DetectorState::PackagePresent: return "Package detected";
  }
  return "Unknown";
}

void recordEvent(const char* label) {
  char entry[120];
  snprintf(entry, sizeof(entry), "%lu ms: %s",
           static_cast<unsigned long>(simulation.samples() * 100), label);
  Serial.printf("SIMULATION: %s\n", entry);
  // Keep the latest six entries; these contain only fixed program labels.
  if (eventHistory.length()) eventHistory += "|";
  eventHistory += entry;
  unsigned int separators = 0;
  for (unsigned int i = 0; i < eventHistory.length(); ++i) {
    if (eventHistory[i] == '|') ++separators;
  }
  if (separators >= 6) eventHistory.remove(0, eventHistory.indexOf('|') + 1);
}

// Local network experiment only: no distance sensor is attached yet.
const char kPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Simulation</title>
<style>
body{font-family:system-ui,sans-serif;background:#edf2f5;color:#172b38;margin:0;padding:32px 20px}
main{max-width:580px;margin:auto;background:white;padding:28px;border-radius:16px}
h1{margin-top:0}p{line-height:1.6}.label{color:#506474}
dl{display:grid;grid-template-columns:1fr 1fr;gap:18px;margin:28px 0}
dd{margin:0;font-weight:600}#connection{font-weight:600;color:#215e51}
.note{background:#f2f5f7;border-radius:8px;padding:14px;font-size:14px}
.badge{display:inline-block;background:#fff0cd;color:#78500a;padding:6px 12px;border-radius:20px;font-weight:700}
#detector{font-size:24px;font-weight:700}button{padding:10px 16px;border:0;border-radius:8px;background:#215e51;color:white;cursor:pointer}
li{margin:8px 0}#events{padding-left:22px;font-size:14px}
</style></head><body><main>
<p class="badge">SIMULATION MODE / 模拟模式</p>
<h1>Smart Package Detector</h1><p id="connection" role="status">Checking connection...</p>
<p id="detector" role="status">--</p>
<p id="phase">--</p>
<dl><dt>Simulated distance</dt><dd id="distance">--</dd>
<dt>Filtered distance</dt><dd id="filtered">--</dd>
<dt>Empty-door baseline</dt><dd id="baseline">--</dd>
<dt>Arrivals / removals</dt><dd id="counts">--</dd>
<dt>Running for</dt><dd id="uptime">--</dd>
<dt>Wi-Fi signal</dt><dd id="rssi">--</dd>
<dt>Last response</dt><dd id="updated">--</dd></dl>
<p class="note">No VL53L0X is connected. Distance is simulated on ESP32;
the existing detector algorithm calculates the result. This is not a real delivery alert.</p>
<p>Repeats: empty door (800 mm, 8 s) → object placed (450 mm, 8 s) → object removed (800 mm, 8 s).</p>
<p class="label">The first 3 seconds learn the baseline. Filtering and confirmation delay state changes.</p>
<button id="restart" type="button">Restart simulation</button>
<p id="action" role="status"></p>
<h2>Recent simulation events</h2><ul id="events"><li>Waiting for data...</li></ul>
<p class="label">Updates every 0.5 seconds. Event times start from simulation reset.</p>
</main><script>
async function refresh(){
  const controller=new AbortController();
  const timeout=setTimeout(()=>controller.abort(),4000);
  try {
    const response=await fetch('/api/status',{cache:'no-store',signal:controller.signal});
    if(!response.ok)throw new Error('Status request failed');
    const s=await response.json();
    document.getElementById('connection').textContent=s.wifi_connected?'Connected to ESP32':'Wi-Fi disconnected';
    document.getElementById('uptime').textContent=s.uptime_seconds+' seconds';
    document.getElementById('rssi').textContent=s.rssi_dbm+' dBm';
    document.getElementById('updated').textContent=new Date().toLocaleTimeString();
    document.getElementById('detector').textContent=s.detector_state;
    document.getElementById('phase').textContent=['Input: empty door','Input: object placed','Input: object removed'][s.phase];
    document.getElementById('distance').textContent=s.distance_mm+' mm';
    document.getElementById('filtered').textContent=s.detector_state==='Calibrating'?'Learning...':s.filtered_mm+' mm';
    document.getElementById('baseline').textContent=s.detector_state==='Calibrating'?'Learning...':s.baseline_mm+' mm';
    document.getElementById('counts').textContent=s.arrivals+' / '+s.removals;
    const events=document.getElementById('events');events.replaceChildren();
    (s.events?s.events.split('|'):['Waiting for calibration...']).forEach(text=>{
      const li=document.createElement('li');li.textContent=text;events.appendChild(li);
    });
  }catch(e){
    document.getElementById('connection').textContent='ESP32 is unreachable. Check power and Wi-Fi.';
    document.getElementById('uptime').textContent='--';
    document.getElementById('rssi').textContent='--';
    for(const id of ['detector','phase','distance','filtered','baseline','counts'])document.getElementById(id).textContent='--';
    document.getElementById('events').textContent='Live data unavailable.';
  }finally{clearTimeout(timeout);setTimeout(refresh,500);}
}
document.getElementById('restart').addEventListener('click',async()=>{
  const button=document.getElementById('restart');button.disabled=true;
  try{
    const response=await fetch('/api/simulation/reset',{method:'POST',signal:AbortSignal.timeout(4000)});
    if(!response.ok)throw new Error('Reset failed');
    document.getElementById('action').textContent='Simulation restarted; learning the baseline.';
  }catch(e){document.getElementById('action').textContent='Reset not confirmed. Check connection.';}
  finally{button.disabled=false;}
});
refresh();
</script></body></html>)HTML";

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nESP32 local status page");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi...");

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html; charset=utf-8", kPage);
  });
  server.on("/api/status", HTTP_GET, []() {
    const uint64_t seconds = esp_timer_get_time() / 1000000ULL;
    char json[1400];
    snprintf(json, sizeof(json),
             "{\"uptime_seconds\":%llu,\"wifi_connected\":%s,"
             "\"rssi_dbm\":%ld,\"sensor_connected\":false,\"mode\":\"simulation\","
             "\"detector_state\":\"%s\",\"phase\":%u,\"distance_mm\":%u,"
             "\"filtered_mm\":%u,\"baseline_mm\":%u,\"arrivals\":%lu,"
             "\"removals\":%lu,\"events\":\"%s\"}",
             static_cast<unsigned long long>(seconds),
             WiFi.status() == WL_CONNECTED ? "true" : "false",
             static_cast<long>(WiFi.RSSI()), stateName(), simulation.phase(),
             simulation.distanceMm(), simulation.detector().filteredMm(),
             simulation.detector().baselineMm(),
             static_cast<unsigned long>(simulation.arrivals()),
             static_cast<unsigned long>(simulation.removals()), eventHistory.c_str());
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
  });
  server.on("/api/simulation/reset", HTTP_POST, []() {
    simulation = Simulation{};
    eventHistory = "";
    lastSampleAt = millis();
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", "{\"reset\":true}");
  });
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
}

void loop() {
  if (millis() - lastSampleAt >= 100) {
    lastSampleAt = millis();
    switch (simulation.tick()) {
      case DetectorEvent::CalibrationComplete: recordEvent("Baseline ready"); break;
      case DetectorEvent::PackageDetected: recordEvent("Package detected"); break;
      case DetectorEvent::PackageRemoved: recordEvent("Package removed"); break;
      case DetectorEvent::None: break;
    }
  }
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !wasConnected) {
    Serial.print("WiFi connected! Open http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
  } else if (!connected && wasConnected) {
    Serial.println("Wi-Fi disconnected. Reconnecting...");
  }
  wasConnected = connected;
  if (connected) {
    server.handleClient();
  } else if (millis() - lastReconnectAt >= 15000) {
    lastReconnectAt = millis();
    Serial.println("Still waiting for Wi-Fi...");
    WiFi.reconnect();
  }
  delay(2);
}
