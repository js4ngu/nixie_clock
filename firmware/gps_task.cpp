#include "gps_task.h"

#include <Arduino.h>
#include <TinyGPSPlus.h>

#include "app_state.h"
#include "debug_config.h"

namespace {

  TaskHandle_t gpsTaskHandle = nullptr;

  TinyGPSPlus gps;

  // ===== GPS UART 설정 =====
  constexpr int GPS_RX_PIN = 19;   // ESP32 RX <- GPS TX
  constexpr int GPS_TX_PIN = 18;   // ESP32 TX -> GPS RX
  constexpr int GPS_BAUD   = 9600;

  HardwareSerial gpsSerial(1);

  // ===== 내부 상태 =====
  bool hasReceivedData = false;

  ClockData toClockData() {
    ClockData t;
    t.year   = gps.date.year();
    t.month  = gps.date.month();
    t.day    = gps.date.day();
    t.hour   = gps.time.hour();
    t.minute = gps.time.minute();
    t.second = gps.time.second();
    return t;
  }

  void gpsTask(void *pvParameters) {
    (void)pvParameters;

    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

    GPS_DEBUG_PRINTLN("[GPS TASK] started");

    while (1) {

      int bytesThisLoop = 0;

      // ===== UART 읽기 =====
      while (gpsSerial.available()) {
        char c = gpsSerial.read();
        gps.encode(c);
        bytesThisLoop++;
      }

      if (bytesThisLoop > 0) {
        hasReceivedData = true;
      }

      // ===== 상태 업데이트 =====
      setGpsReceiving(hasReceivedData);

      uint8_t sats = gps.satellites.isValid() ? gps.satellites.value() : 0;
      setGpsSatellites(sats);

      // ===== VALID 판정 =====
      bool validDate = gps.date.isValid();
      bool validTime = gps.time.isValid();
      bool validLoc  = gps.location.isValid();

      bool gpsLocked =
          validDate &&
          validTime &&
          validLoc;

      setGpsLocked(gpsLocked);

      // ===== DEBUG =====
      GPS_DEBUG_PRINTF(
        "[GPS RAW] date=%d time=%d loc=%d sats=%d bytes=%d\n",
        validDate,
        validTime,
        validLoc,
        sats,
        bytesThisLoop
      );

      // ===== 시간 처리 =====
      if (gpsLocked) {

        ClockData t = toClockData();

        setCurrentTime(t);
        setTimeSource(TIME_SOURCE_GPS);

        GPS_DEBUG_PRINTF(
          "[GPS TASK] UTC %04d-%02d-%02d %02d:%02d:%02d\n",
          t.year,
          t.month,
          t.day,
          t.hour,
          t.minute,
          t.second
        );

      } else {

        GPS_DEBUG_PRINTLN("[GPS TASK] UTC invalid");

        // GPS 안잡히면 RTC 유지 (RTC task가 이미 돌고 있음)
        setTimeSource(TIME_SOURCE_RTC);
      }

      // ===== 최초 데이터 없음 경고 =====
      if (!hasReceivedData) {
        GPS_DEBUG_PRINTLN("[GPS TASK] No GPS data detected yet");
      }

      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

void startGPSTask() {
  xTaskCreatePinnedToCore(
    gpsTask,
    "TaskGPS",
    4096,
    nullptr,
    1,
    &gpsTaskHandle,
    1
  );
}
