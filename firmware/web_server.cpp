#include "web_server.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "app_state.h"
#include "settings_store.h"
#include "debug_config.h"
#include "debug_log.h"
#include "rtc_task.h"

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

  String getDimmingText(const AppState& state) {
    return String(state.dimming) + "%";
  }

  String getDivergencePeriodText(const AppState& state) {
    const uint32_t period = state.divergencePeriod;

    if (period % 3600 == 0) {
      return String(period / 3600) + " hour";
    }

    if (period % 60 == 0) {
      return String(period / 60) + " minutes";
    }

    return String(period) + " sec";
  }

  String escapeJson(const String& input) {
    String output;
    output.reserve(input.length() + 32);

    for (size_t i = 0; i < input.length(); ++i) {
      char c = input[i];
      switch (c) {
        case '\\': output += "\\\\"; break;
        case '"': output += "\\\""; break;
        case '\n': output += "\\n"; break;
        case '\r': output += "\\r"; break;
        case '\t': output += "\\t"; break;
        default: output += c; break;
      }
    }

    return output;
  }

  bool parseUintArg(const char* name, int& outValue) {
    if (!server.hasArg(name)) {
      return false;
    }

    outValue = server.arg(name).toInt();
    return true;
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
    .status-value {
      font-size: 30px;
      font-weight: 800;
      margin-bottom: 8px;
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
    .log-box {
      margin-top: 12px;
      background: #0b0b0c;
      color: #9af7a2;
      border-radius: 12px;
      padding: 14px;
      min-height: 220px;
      max-height: 320px;
      overflow-y: auto;
      font: 12px/1.45 "SFMono-Regular", Consolas, "Liberation Mono", monospace;
      white-space: pre-wrap;
      word-break: break-word;
    }
    .log-box.rtc {
      color: #7ed7ff;
    }
    input[type=datetime-local] {
      width: 100%;
      padding: 12px;
      border-radius: 12px;
      border: none;
      font-size: 16px;
      background: #2c2c2e;
      color: #fff;
      margin-top: 10px;
      box-sizing: border-box;
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
      <div class="label">CURRENT DIMMING</div>
      <div class="status-value" id="dimmingStatus">__DIMMING_TEXT__</div>
      <div class="hint">현재 저장된 디밍 값입니다.</div>
    </div>

    <div class="card">
      <div class="label">DIVERGENCE MODE PERIOD</div>
      <div class="status-value" id="divergencePeriodStatus">__DIVERGENCE_PERIOD_TEXT__</div>
      <div class="hint">현재 저장된 다이버전스 모드 주기입니다.</div>
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

    <div class="card">
      <div class="label">RTC SYNC</div>
      <button onclick="syncBrowserTime()">브라우저 시간으로 동기화</button>
      <div class="hint">현재 접속한 폰 또는 PC 시간을 DS3231 RTC에 반영합니다.</div>
    </div>

    <div class="card">
      <div class="label">RTC MANUAL SET</div>
      <input type="datetime-local" id="manualRtcTime">
      <button onclick="applyManualRtcTime()">수동 시간 적용</button>
      <div class="hint">GPS 수신이 안 될 때 수동으로 RTC 시간을 맞출 수 있습니다.</div>
    </div>

    <div class="card">
      <div class="label">RTC LOG</div>
      <div class="hint">RTC 초기화, 동기화, 현재 시각 갱신 로그만 별도로 표시합니다.</div>
      <pre class="log-box rtc" id="rtcLogs">loading...</pre>
    </div>

    <div class="card">
      <div class="label">DEBUG LOG</div>
      <div class="hint">최근 디버그 로그만 보관됩니다. GPS 수신 상태를 실시간으로 확인할 수 있습니다.</div>
      <pre class="log-box" id="debugLogs">loading...</pre>
    </div>
  </div>

  <script>
    const slider = document.getElementById('dimmingSlider');
    const dimmingValue = document.getElementById('dimmingValue');
    const gpsStatus = document.getElementById('gpsStatus');
    const signalQuality = document.getElementById('signalQuality');
    const satellites = document.getElementById('satellites');
    const timeSource = document.getElementById('timeSource');
    const dimmingStatus = document.getElementById('dimmingStatus');
    const divergencePeriodStatus = document.getElementById('divergencePeriodStatus');
    const periodSelect = document.getElementById('divergencePeriod');
    const manualRtcTime = document.getElementById('manualRtcTime');
    const rtcLogs = document.getElementById('rtcLogs');
    const debugLogs = document.getElementById('debugLogs');

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

    async function setRtcTime(date) {
      const params = new URLSearchParams({
        year: String(date.getFullYear()),
        month: String(date.getMonth() + 1),
        day: String(date.getDate()),
        hour: String(date.getHours()),
        minute: String(date.getMinutes()),
        second: String(date.getSeconds())
      });

      const res = await fetch('/api/set-time?' + params.toString());
      if (!res.ok) {
        throw new Error(await res.text());
      }

      await refreshAll();
    }

    async function syncBrowserTime() {
      try {
        await setRtcTime(new Date());
      } catch (e) {
        console.log(e);
      }
    }

    async function applyManualRtcTime() {
      if (!manualRtcTime.value) {
        return;
      }

      try {
        await setRtcTime(new Date(manualRtcTime.value));
      } catch (e) {
        console.log(e);
      }
    }

    async function refreshState() {
      try {
        const res = await fetch('/api/state');
        const data = await res.json();

        gpsStatus.textContent = data.gpsStatusText;
        signalQuality.textContent = data.signalQualityText;
        satellites.textContent = data.gpsSatellites;
        timeSource.textContent = data.timeSourceText;
        dimmingStatus.textContent = data.dimmingText;
        divergencePeriodStatus.textContent = data.divergencePeriodText;

        slider.value = data.dimming;
        dimmingValue.textContent = data.dimming;
        periodSelect.value = String(data.divergencePeriod);
      } catch (e) {
        console.log(e);
      }
    }

    async function refreshRtcLogs() {
      try {
        const shouldStick = rtcLogs.scrollTop + rtcLogs.clientHeight >= rtcLogs.scrollHeight - 20;
        const res = await fetch('/api/rtc-logs');
        const data = await res.json();

        rtcLogs.textContent = data.logs || '';

        if (shouldStick) {
          rtcLogs.scrollTop = rtcLogs.scrollHeight;
        }
      } catch (e) {
        console.log(e);
      }
    }

    async function refreshLogs() {
      try {
        const shouldStick = debugLogs.scrollTop + debugLogs.clientHeight >= debugLogs.scrollHeight - 20;
        const res = await fetch('/api/logs');
        const data = await res.json();

        debugLogs.textContent = data.logs || '';

        if (shouldStick) {
          debugLogs.scrollTop = debugLogs.scrollHeight;
        }
      } catch (e) {
        console.log(e);
      }
    }

    async function refreshAll() {
      await Promise.all([refreshState(), refreshRtcLogs(), refreshLogs()]);
    }

    window.addEventListener('load', refreshAll);
    setInterval(refreshAll, 2000);
  </script>
</body>
</html>
)rawliteral";

    html.replace("__GPS_STATUS__", getGpsStatusText(state));
    html.replace("__SIGNAL_QUALITY__", getSignalQualityText(state));
    html.replace("__SATELLITES__", String(state.gpsSatellites));
    html.replace("__TIME_SOURCE__", getTimeSourceText(state));
    html.replace("__DIMMING__", String(state.dimming));
    html.replace("__DIMMING_TEXT__", getDimmingText(state));
    html.replace("__DIVERGENCE_PERIOD_TEXT__", getDivergencePeriodText(state));

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
    json += "\"dimmingText\":\"" + getDimmingText(state) + "\",";
    json += "\"divergencePeriod\":" + String(state.divergencePeriod) + ",";
    json += "\"divergencePeriodText\":\"" + getDivergencePeriodText(state) + "\"";
    json += "}";

    server.send(200, "application/json", json);
  }

  void handleApiRtcLogs() {
    String logs;
    getRtcLogSnapshot(logs);

    String json = "{";
    json += "\"logs\":\"" + escapeJson(logs) + "\"";
    json += "}";

    server.send(200, "application/json", json);
  }

  void handleApiSetTime() {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (!parseUintArg("year", year) ||
        !parseUintArg("month", month) ||
        !parseUintArg("day", day) ||
        !parseUintArg("hour", hour) ||
        !parseUintArg("minute", minute) ||
        !parseUintArg("second", second)) {
      server.send(400, "text/plain", "missing time field");
      return;
    }

    bool valid =
      year >= 2024 && year <= 2099 &&
      month >= 1 && month <= 12 &&
      day >= 1 && day <= 31 &&
      hour >= 0 && hour <= 23 &&
      minute >= 0 && minute <= 59 &&
      second >= 0 && second <= 59;

    if (!valid) {
      server.send(400, "text/plain", "invalid time field");
      return;
    }

    if (!setRTCFromLocalTime(
      static_cast<uint16_t>(year),
      static_cast<uint8_t>(month),
      static_cast<uint8_t>(day),
      static_cast<uint8_t>(hour),
      static_cast<uint8_t>(minute),
      static_cast<uint8_t>(second)
    )) {
      server.send(503, "text/plain", "rtc not ready");
      return;
    }

    server.send(200, "text/plain", "OK");
  }

  void handleApiLogs() {
    String logs;
    getDebugLogSnapshot(logs);

    String json = "{";
    json += "\"logs\":\"" + escapeJson(logs) + "\"";
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
  server.on("/api/rtc-logs", HTTP_GET, handleApiRtcLogs);
  server.on("/api/logs", HTTP_GET, handleApiLogs);
  server.on("/api/set-time", HTTP_GET, handleApiSetTime);
  server.on("/api/dimming", HTTP_GET, handleApiDimming);
  server.on("/api/period", HTTP_GET, handleApiPeriod);
  server.onNotFound(handleNotFound);
  server.begin();

  WEB_TASK_DEBUG_PRINTLN("[WEB] HTTP server started");

  startWebServerTask();
}
