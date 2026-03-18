#include <Arduino.h>

#include "app_state.h"
#include "settings_store.h"
#include "web_server.h"
#include "gps_task.h"
#include "rtc_task.h"
#include "display_task.h"
#include "debug_log.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  initDebugLog();

  debugLogPrintln();
  debugLogPrintln("=== BOOT ===");

  initAppState();
  loadSettings();

  startWebServer();
  startGPSTask();
  startRTCTask();
  startDisplayTask();
}

void loop() {
  delay(1000);
}
