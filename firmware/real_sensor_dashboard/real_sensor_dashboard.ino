#include <Adafruit_VL53L0X.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_timer.h>
#include <cstring>

#include "desktop_test_config.h"
#include "package_detector.h"
#include "sample_log.h"
#include "secrets.h"

namespace {
constexpr uint32_t kSampleIntervalMs = 100;
constexpr size_t kMaxEvents = 6;

Adafruit_VL53L0X sensor;
WebServer server(80);
PackageDetector detector(desktopTestConfig());
SampleLog sampleLog;
bool sensorConnected = false;
bool wasWifiConnected = false;
bool hasValidReading = false;
uint16_t distanceMm = 0;
uint8_t lastRangeStatus = 255;
uint32_t validSamples = 0;
uint32_t invalidSamples = 0;
uint32_t arrivals = 0;
uint32_t removals = 0;
uint32_t lastSampleAt = 0;
uint32_t lastReconnectAt = 0;
uint32_t lastSensorRetryAt = 0;
String eventHistory;

void appendSample(SampleQuality quality, uint16_t rawMm = 0,
                  int16_t apiError = 0, uint8_t rangeStatus = 255,
                  DetectorEvent event = DetectorEvent::None,
                  ExperimentMarker marker = ExperimentMarker::None) {
  SampleRecord record;
  record.uptimeMs = esp_timer_get_time() / 1000ULL;
  record.quality = quality;
  record.rawMm = rawMm;
  record.apiError = apiError;
  record.rangeStatus = rangeStatus;
  record.filteredMm = detector.filteredMm();
  record.baselineMm = detector.baselineMm();
  record.state = detector.state();
  record.event = event;
  record.marker = marker;
  sampleLog.append(record);
}

const char* stateName() {
  switch (detector.state()) {
    case DetectorState::Calibrating: return "Calibrating";
    case DetectorState::Clear: return "No package";
    case DetectorState::PackagePresent: return "Package detected";
  }
  return "Unknown";
}

void recordEvent(const char* label) {
  char entry[100];
  snprintf(entry, sizeof(entry), "%lu s: %s",
           static_cast<unsigned long>(millis() / 1000), label);
  Serial.printf("EVENT: %s\n", entry);
  if (eventHistory.length()) eventHistory += '|';
  eventHistory += entry;
  size_t separators = 0;
  for (size_t i = 0; i < eventHistory.length(); ++i) {
    if (eventHistory[i] == '|') ++separators;
  }
  if (separators >= kMaxEvents) {
    eventHistory.remove(0, eventHistory.indexOf('|') + 1);
  }
}

void resetDetector() {
  detector = PackageDetector(desktopTestConfig());
  hasValidReading = false;
  distanceMm = 0;
  lastRangeStatus = 255;
  validSamples = 0;
  invalidSamples = 0;
  eventHistory = "";
  recordEvent("Calibration restarted");
  appendSample(SampleQuality::CalibrationReset);
}

void sampleSensor() {
  if (!sensorConnected) {
    hasValidReading = false;
    distanceMm = 0;
    lastRangeStatus = 255;
    appendSample(SampleQuality::SensorOffline);
    const uint32_t now = millis();
    if (now - lastSensorRetryAt >= 2000) {
      lastSensorRetryAt = now;
      sensorConnected = sensor.begin();
      if (sensorConnected) {
        Serial.println("VL53L0X reconnected. Recalibrating...");
        resetDetector();
      }
    }
    return;
  }
  VL53L0X_RangingMeasurementData_t measurement{};
  const VL53L0X_Error error =
      sensor.getSingleRangingMeasurement(&measurement, false);
  if (error != VL53L0X_ERROR_NONE) {
    ++invalidSamples;
    hasValidReading = false;
    distanceMm = 0;
    lastRangeStatus = 255;
    appendSample(SampleQuality::ApiError, 0, static_cast<int16_t>(error));
    return;
  }
  lastRangeStatus = measurement.RangeStatus;
  if (lastRangeStatus != 0) {
    ++invalidSamples;
    hasValidReading = false;
    distanceMm = 0;
    appendSample(SampleQuality::RangeInvalid, measurement.RangeMilliMeter,
                 0, lastRangeStatus);
    return;
  }

  hasValidReading = true;
  distanceMm = measurement.RangeMilliMeter;
  ++validSamples;
  const DetectorEvent event = detector.update(distanceMm);
  switch (event) {
    case DetectorEvent::CalibrationComplete:
      recordEvent("Baseline ready");
      break;
    case DetectorEvent::PackageDetected:
      ++arrivals;
      recordEvent("Package detected");
      break;
    case DetectorEvent::PackageRemoved:
      ++removals;
      recordEvent("Package removed");
      break;
    case DetectorEvent::None:
      break;
  }
  appendSample(SampleQuality::Valid, distanceMm, 0, 0, event);
}

const char kPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Package Detector</title><style>
body{font-family:system-ui,sans-serif;background:#edf2f5;color:#172b38;margin:0;padding:32px 20px}
main{max-width:620px;margin:auto;background:#fff;padding:28px;border-radius:16px;box-shadow:0 8px 28px #18313e18}
h1{margin:10px 0}.badge{display:inline-block;background:#dff4e8;color:#155d3a;padding:6px 12px;border-radius:20px;font-weight:700}
#detector{font-size:27px;font-weight:750}.ok{color:#16704a}.alert{color:#a33a24}.label{color:#506474}
dl{display:grid;grid-template-columns:1fr 1fr;gap:18px;margin:28px 0}dd{margin:0;font-weight:650}
.note{background:#f2f5f7;border-radius:8px;padding:14px;font-size:14px;line-height:1.5}
button{padding:11px 17px;border:0;border-radius:8px;background:#215e51;color:#fff;font-weight:650;cursor:pointer}
 a.download{display:inline-block;margin-top:12px;padding:11px 17px;border-radius:8px;background:#edf2f5;color:#172b38;font-weight:650;text-decoration:none}
 .markers{display:flex;flex-wrap:wrap;gap:8px;margin-top:14px}.markers button{background:#3f6172}
button:disabled{opacity:.55}li{margin:8px 0}#events{padding-left:22px;font-size:14px}
</style></head><body><main>
<span class="badge">LIVE SENSOR / 真实传感器</span><h1>Smart Package Detector</h1>
<p id="connection" role="status">Connecting...</p><p id="detector" role="status">--</p>
<dl><dt>Live distance / 当前距离</dt><dd id="distance">--</dd>
<dt>Filtered / 滤波距离</dt><dd id="filtered">--</dd>
<dt>Empty baseline / 空背景基线</dt><dd id="baseline">--</dd>
<dt>Arrivals / removals</dt><dd id="counts">--</dd>
<dt>Valid / invalid samples</dt><dd id="samples">--</dd>
<dt>Recorded rows</dt><dd id="recorded">--</dd>
<dt>Wi-Fi signal</dt><dd id="rssi">--</dd>
<dt>Running for</dt><dd id="uptime">--</dd>
<dt>Last update</dt><dd id="updated">--</dd></dl>
<p class="note">Tabletop test profile: an object must stay at least 60 mm closer than the learned empty background for about 5 seconds before detection. Brief passers-by are ignored. Keep the scene empty during the first 3 seconds after calibration starts.</p>
<button id="recalibrate" type="button">Recalibrate empty background / 重新校准</button>
<div class="markers"><button type="button" data-marker="hand_pass">Mark hand pass</button>
<button type="button" data-marker="object_placed">Mark object placed</button>
<button type="button" data-marker="object_removed">Mark object removed</button></div>
<br><a class="download" href="/api/samples.csv" download="sensor-samples.csv">Download measurements CSV / 下载测量数据</a>
<p class="label">Press a marker as you perform each test action. Mark times are approximate. CSV keeps the latest ~2 minutes in memory; timestamps are milliseconds since ESP32 startup. Download before rebooting.</p>
<p id="action" role="status"></p><h2>Recent events / 最近事件</h2>
<ul id="events"><li>Waiting for readings...</li></ul></main><script>
function value(id,text){document.getElementById(id).textContent=text}
async function refresh(){const c=new AbortController(),t=setTimeout(()=>c.abort(),4000);try{
 const r=await fetch('/api/status',{cache:'no-store',signal:c.signal});if(!r.ok)throw Error();const s=await r.json();
 value('connection',s.wifi_connected&&s.sensor_connected?'ESP32 and VL53L0X connected':'Hardware connection problem');
 const d=document.getElementById('detector');d.textContent=s.detector_state;d.className=s.detector_state==='Package detected'?'alert':'ok';
 value('distance',s.has_valid_reading?s.distance_mm+' mm':'Waiting for valid return');
 value('filtered',s.detector_state==='Calibrating'?'Learning...':s.filtered_mm+' mm');
 value('baseline',s.detector_state==='Calibrating'?'Learning...':s.baseline_mm+' mm');
 value('counts',s.arrivals+' / '+s.removals);value('samples',s.valid_samples+' / '+s.invalid_samples);
 value('recorded',s.recorded_rows+' / '+s.record_capacity);
 value('rssi',s.rssi_dbm+' dBm');value('uptime',s.uptime_seconds+' seconds');value('updated',new Date().toLocaleTimeString());
 const e=document.getElementById('events');e.replaceChildren();(s.events?s.events.split('|'):['Waiting for calibration...']).forEach(x=>{const li=document.createElement('li');li.textContent=x;e.appendChild(li)});
}catch(e){value('connection','ESP32 is unreachable. Check power and Wi-Fi.');for(const id of ['detector','distance','filtered','baseline','counts','samples','recorded','rssi','uptime','updated'])value(id,'--')}finally{clearTimeout(t);setTimeout(refresh,500)}}
document.getElementById('recalibrate').addEventListener('click',async()=>{const b=document.getElementById('recalibrate');b.disabled=true;try{const r=await fetch('/api/calibrate',{method:'POST'});if(!r.ok)throw Error();value('action','Calibration restarted. Keep the view empty for 3 seconds.')}catch(e){value('action','Calibration request failed.')}finally{b.disabled=false}});refresh();
document.querySelectorAll('[data-marker]').forEach(b=>b.addEventListener('click',async()=>{b.disabled=true;try{const r=await fetch('/api/mark?kind='+encodeURIComponent(b.dataset.marker),{method:'POST'});if(!r.ok)throw Error();value('action','Marked '+b.dataset.marker+' in CSV.')}catch(e){value('action','Marker request failed.')}finally{b.disabled=false}}));
</script></body></html>)HTML";
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nReal VL53L0X Wi-Fi dashboard");
  sensorConnected = sensor.begin();
  Serial.println(sensorConnected ? "VL53L0X online." : "ERROR: VL53L0X not found.");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html; charset=utf-8", kPage);
  });
  server.on("/api/status", HTTP_GET, []() {
    char json[1600];
    snprintf(json, sizeof(json),
      "{\"uptime_seconds\":%llu,\"wifi_connected\":%s,\"rssi_dbm\":%ld,"
      "\"sensor_connected\":%s,\"has_valid_reading\":%s,\"range_status\":%u,"
      "\"detector_state\":\"%s\",\"distance_mm\":%u,\"filtered_mm\":%u,"
      "\"baseline_mm\":%u,\"valid_samples\":%lu,\"invalid_samples\":%lu,"
      "\"arrivals\":%lu,\"removals\":%lu,\"recorded_rows\":%lu,"
      "\"record_capacity\":%lu,\"events\":\"%s\"}",
      static_cast<unsigned long long>(esp_timer_get_time() / 1000000ULL),
      WiFi.status() == WL_CONNECTED ? "true" : "false", static_cast<long>(WiFi.RSSI()),
      sensorConnected ? "true" : "false", hasValidReading ? "true" : "false",
      lastRangeStatus, stateName(), distanceMm, detector.filteredMm(), detector.baselineMm(),
      static_cast<unsigned long>(validSamples), static_cast<unsigned long>(invalidSamples),
      static_cast<unsigned long>(arrivals), static_cast<unsigned long>(removals),
      static_cast<unsigned long>(sampleLog.size()),
      static_cast<unsigned long>(SampleLog::kCapacity), eventHistory.c_str());
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
  });
  server.on("/api/calibrate", HTTP_POST, []() {
    resetDetector();
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", "{\"calibrating\":true}");
  });
  server.on("/api/mark", HTTP_POST, []() {
    const String kind = server.arg("kind");
    ExperimentMarker marker = ExperimentMarker::None;
    if (kind == "hand_pass") marker = ExperimentMarker::HandPass;
    else if (kind == "object_placed") marker = ExperimentMarker::ObjectPlaced;
    else if (kind == "object_removed") marker = ExperimentMarker::ObjectRemoved;
    else {
      server.send(400, "application/json", "{\"error\":\"unknown marker\"}");
      return;
    }
    appendSample(SampleQuality::ManualMarker,
                 hasValidReading ? distanceMm : 0, 0, lastRangeStatus,
                 DetectorEvent::None, marker);
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", "{\"marked\":true}");
  });
  server.on("/api/samples.csv", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("Content-Disposition",
                      "attachment; filename=\"sensor-samples.csv\"");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/csv; charset=utf-8", "");
    const char header[] =
        "uptime_ms,quality,api_error,range_status,raw_mm,filtered_mm,baseline_mm,state,event,marker\n";
    server.sendContent(header, sizeof(header) - 1);

    char block[512];
    size_t blockUsed = 0;
    const size_t rowCount = sampleLog.size();
    for (size_t i = 0; i < rowCount; ++i) {
      char row[160];
      const int length = formatSampleCsvRow(row, sizeof(row), sampleLog.oldest(i));
      if (length <= 0 || static_cast<size_t>(length) >= sizeof(row)) continue;
      const size_t rowLength = static_cast<size_t>(length);
      if (blockUsed + rowLength > sizeof(block)) {
        server.sendContent(block, blockUsed);
        blockUsed = 0;
      }
      std::memcpy(block + blockUsed, row, rowLength);
      blockUsed += rowLength;
    }
    if (blockUsed) server.sendContent(block, blockUsed);
    server.sendContent("", 0);
  });
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
}

void loop() {
  const uint32_t now = millis();
  if (now - lastSampleAt >= kSampleIntervalMs) {
    lastSampleAt = now;
    sampleSensor();
  }

  const bool wifiConnected = WiFi.status() == WL_CONNECTED;
  if (wifiConnected && !wasWifiConnected) {
    Serial.print("Open http://"); Serial.print(WiFi.localIP()); Serial.println('/');
  } else if (!wifiConnected && wasWifiConnected) {
    Serial.println("Wi-Fi disconnected. Reconnecting...");
  }
  wasWifiConnected = wifiConnected;
  if (wifiConnected) {
    server.handleClient();
  } else if (now - lastReconnectAt >= 15000) {
    lastReconnectAt = now;
    WiFi.reconnect();
  }
  delay(2);
}
