#include "web_server.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "app_state.h"
#include "settings_store.h"
#include "debug_config.h"

namespace {
  const char* AP_SSID = "NixieClock";
  const char* AP_PASS = "12345678";

  WebServer server(80);
  TaskHandle_t webServerTaskHandle = nullptr;

  String getGpsStatusText(const AppState& state) {
    if (state.gpsLocked && state.timeSource == TIME_SOURCE_GPS) {
      return "GPS SYNCED";
    }

    // GPS 시간이 유효한 상태
    if (state.gpsLocked) {
      return "TIME RECEIVED";
    }

    if (state.gpsReceiving) {
      return "NOW RECEIVING...";
    }

    if (!state.gpsReceiving && state.gpsEverLocked) {
      return "SIGNAL LOST";
    }

    return "SEARCHING SIGNAL...";
  }

  String getSignalQualityText(const AppState& state) {
    if (!state.gpsReceiving) {
      return "No Signal";
    }

    if (state.gpsSatellites >= 7) {
      return "Good";
    } else if (state.gpsSatellites >= 4) {
      return "Medium";
    } else if (state.gpsSatellites >= 1) {
      return "Weak";
    }

    if (state.gpsLocked) {
      return "Time Only";
    }

    return "Receiving";
  }

  String getTimeSourceText(const AppState& state) {
    return (state.timeSource == TIME_SOURCE_GPS) ? "GPS" : "RTC";
  }

  String makeHtmlPage(const AppState& state) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="ko">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Nixie Clock</title>
  <style>
    body {
      margin: 0;
      padding: 0;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: #111;
      color: #f5f5f5;
    }
    .wrap {
      max-width: 480px;
      margin: 0 auto;
      padding: 20px;
    }
    .card {
      background: #1c1c1e;
      border-radius: 16px;
      padding: 18px;
      margin-bottom: 16px;
      box-shadow: 0 4px 16px rgba(0,0,0,0.25);
    }
    h1 {
      font-size: 24px;
      margin: 0 0 18px 0;
    }
    .label {
      font-size: 14px;
      color: #b0b0b0;
      margin-bottom: 8px;
    }
    .value {
      font-size: 24px;
      font-weight: 700;
      margin-bottom: 12px;
    }
    .subvalue {
      font-size: 16px;
      margin-bottom: 6px;
      color: #d0d0d0;
    }
    .slider-value {
      margin-top: 10px;
      font-size: 20px;
      font-weight: 700;
    }
    input[type=range] {
      width: 100%;
      margin-top: 10px;
    }
    select {
      width: 100%;
      padding: 12px;
      border-radius: 12px;
      border: none;
      font-size: 16px;
      background: #2c2c2e;
      color: #fff;
      margin-top: 10px;
    }
    button {
      width: 100%;
      padding: 14px;
      margin-top: 14px;
      border: none;
      border-radius: 12px;
      background: #ff7a00;
      color: white;
      font-size: 16px;
      font-weight: 700;
    }
    .hint {
      margin-top: 10px;
      font-size: 13px;
      color: #9a9a9a;
    }
  </style>
</head>
<body>
  <div class="wrap">
    <h1>Nixie Clock Control</h1>

    <div class="card">
      <div class="label">GPS STATUS</div>
      <div class="value" id="gpsStatus">__GPS_STATUS__</div>
      <div class="subvalue">Signal Quality: <span id="signalQuality">__SIGNAL_QUALITY__</span></div>
      <div class="subvalue">Satellites: <span id="satellites">__SATELLITES__</span></div>
      <div class="subvalue">Time Source: <span id="timeSource">__TIME_SOURCE__</span></div>
    </div>

    <div class="card">
      <div class="label">DIMMING</div>
      <input type="range" min="0" max="100" value="__DIMMING__" id="dimmingSlider">
      <div class="slider-value"><span id="dimmingValue">__DIMMING__</span>%</div>
      <button onclick="applyDimming()">디밍 적용</button>
      <div class="hint">전원 재시작 후에도 유지됩니다.</div>
    </div>

    <div class="card">
      <div class="label">DIVERGENCE METER PERIOD</div>
      <select id="divergencePeriod">
        <option value="1800">30 minutes</option>
        <option value="3600">1 hour</option>
        <option value="7200">2 hours</option>
        <option value="14400">4 hours</option>
      </select>
      <button onclick="applyPeriod()">주기 적용</button>
      <div class="hint">다이버전스미터 모드 동작 주기를 설정합니다.</div>
    </div>
  </div>

  <script>
    const slider = document.getElementById('dimmingSlider');
    const dimmingValue = document.getElementById('dimmingValue');
    const gpsStatus = document.getElementById('gpsStatus');
    const signalQuality = document.getElementById('signalQuality');
    const satellites = document.getElementById('satellites');
    const timeSource = document.getElementById('timeSource');
    const periodSelect = document.getElementById('divergencePeriod');

    slider.addEventListener('input', () => {
      dimmingValue.textContent = slider.value;
    });

    async function applyDimming() {
      const value = slider.value;
      await fetch('/api/dimming?value=' + value);
      await refreshState();
    }

    async function applyPeriod() {
      const value = periodSelect.value;
      await fetch('/api/period?value=' + value);
      await refreshState();
    }

    async function refreshState() {
      try {
        const res = await fetch('/api/state');
        const data = await res.json();

        gpsStatus.textContent = data.gpsStatusText;
        signalQuality.textContent = data.signalQualityText;
        satellites.textContent = data.gpsSatellites;
        timeSource.textContent = data.timeSourceText;

        slider.value = data.dimming;
        dimmingValue.textContent = data.dimming;
        periodSelect.value = String(data.divergencePeriod);
      } catch (e) {
        console.log(e);
      }
    }

    window.addEventListener('load', refreshState);
    setInterval(refreshState, 2000);
  </script>
</body>
</html>
)rawliteral";

    html.replace("__GPS_STATUS__", getGpsStatusText(state));
    html.replace("__SIGNAL_QUALITY__", getSignalQualityText(state));
    html.replace("__SATELLITES__", String(state.gpsSatellites));
    html.replace("__TIME_SOURCE__", getTimeSourceText(state));
    html.replace("__DIMMING__", String(state.dimming));

    return html;
  }

  void handleRoot() {
    AppState state;
    getAppStateSnapshot(state);
    server.send(200, "text/html; charset=UTF-8", makeHtmlPage(state));
  }

  void handleApiState() {
    AppState state;
    getAppStateSnapshot(state);

    String json = "{";
    json += "\"gpsStatusText\":\"" + getGpsStatusText(state) + "\",";
    json += "\"signalQualityText\":\"" + getSignalQualityText(state) + "\",";
    json += "\"gpsSatellites\":" + String(state.gpsSatellites) + ",";
    json += "\"timeSourceText\":\"" + getTimeSourceText(state) + "\",";
    json += "\"gpsLocked\":" + String(state.gpsLocked ? "true" : "false") + ",";
    json += "\"gpsReceiving\":" + String(state.gpsReceiving ? "true" : "false") + ",";
    json += "\"gpsEverLocked\":" + String(state.gpsEverLocked ? "true" : "false") + ",";
    json += "\"dimming\":" + String(state.dimming) + ",";
    json += "\"divergencePeriod\":" + String(state.divergencePeriod);
    json += "}";

    server.send(200, "application/json", json);
  }

  void handleApiDimming() {
    if (!server.hasArg("value")) {
      server.send(400, "text/plain", "missing value");
      return;
    }

    int value = server.arg("value").toInt();
    value = constrain(value, 0, 100);

    setDimming((uint8_t)value);
    saveDimmingSetting((uint8_t)value);

    WEB_API_DEBUG_PRINTF("[WEB] dimming updated: %d\n", value);
    server.send(200, "text/plain", "OK");
  }

  void handleApiPeriod() {
    if (!server.hasArg("value")) {
      server.send(400, "text/plain", "missing value");
      return;
    }

    uint32_t value = (uint32_t)server.arg("value").toInt();
    bool valid = (value == 1800 || value == 3600 || value == 7200 || value == 14400);

    if (!valid) {
      server.send(400, "text/plain", "invalid value");
      return;
    }

    setDivergencePeriod(value);
    saveDivergencePeriodSetting(value);

    WEB_API_DEBUG_PRINTF("[WEB] divergence period updated: %lu sec\n", (unsigned long)value);
    server.send(200, "text/plain", "OK");
  }

  void handleNotFound() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  }

  void webServerTask(void *pvParameters) {
    (void)pvParameters;

    while (1) {
      server.handleClient();
      WEB_TASK_DEBUG_PRINTLN("[WEB SERVER TASK] running");
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

void startWebServerTask() {
  xTaskCreatePinnedToCore(
    webServerTask,
    "TaskWebServer",
    6144,
    nullptr,
    1,
    &webServerTaskHandle,
    1
  );
}

void startWebServer() {
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, AP_PASS);

  if (ok) {
    WEB_TASK_DEBUG_PRINTLN("[WIFI] SoftAP started");
    WEB_TASK_DEBUG_PRINT("[WIFI] SSID: ");
    WEB_TASK_DEBUG_PRINTLN(AP_SSID);
    WEB_TASK_DEBUG_PRINT("[WIFI] IP: ");
    WEB_TASK_DEBUG_PRINTLN(WiFi.softAPIP());
  } else {
    WEB_TASK_DEBUG_PRINTLN("[WIFI] SoftAP start failed");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleApiState);
  server.on("/api/dimming", HTTP_GET, handleApiDimming);
  server.on("/api/period", HTTP_GET, handleApiPeriod);
  server.onNotFound(handleNotFound);
  server.begin();

  WEB_TASK_DEBUG_PRINTLN("[WEB] HTTP server started");

  startWebServerTask();
}