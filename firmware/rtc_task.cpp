#include "rtc_task.h"

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

#include "app_state.h"
#include "debug_config.h"

namespace {
  TaskHandle_t rtcTaskHandle = nullptr;
  RTC_DS3231 rtc;
  bool rtcReady = false;

  // ===== RTC I2C 설정 =====
  constexpr int I2C_SDA_PIN = 5;
  constexpr int I2C_SCL_PIN = 17;

  ClockData toClockData(const DateTime& dt) {
    ClockData t;
    t.year   = dt.year();
    t.month  = dt.month();
    t.day    = dt.day();
    t.hour   = dt.hour();
    t.minute = dt.minute();
    t.second = dt.second();
    return t;
  }

  bool isLeapYear(int year) {
    if (year % 400 == 0) return true;
    if (year % 100 == 0) return false;
    return (year % 4 == 0);
  }

  int daysInMonth(int year, int month) {
    static const int days[12] = {
      31, 28, 31, 30, 31, 30,
      31, 31, 30, 31, 30, 31
    };

    if (month == 2 && isLeapYear(year)) {
      return 29;
    }
    return days[month - 1];
  }

  void addHours(
    uint16_t &year,
    uint8_t &month,
    uint8_t &day,
    uint8_t &hour,
    int addHour
  ) {
    int h = (int)hour + addHour;

    while (h >= 24) {
      h -= 24;
      day++;

      int dim = daysInMonth(year, month);
      if (day > dim) {
        day = 1;
        month++;

        if (month > 12) {
          month = 1;
          year++;
        }
      }
    }

    while (h < 0) {
      h += 24;

      if (day > 1) {
        day--;
      } else {
        if (month > 1) {
          month--;
        } else {
          month = 12;
          year--;
        }
        day = daysInMonth(year, month);
      }
    }

    hour = (uint8_t)h;
  }

  void rtcTask(void *pvParameters) {
    (void)pvParameters;

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    RTC_DEBUG_PRINTLN("[RTC TASK] initializing DS3231...");

    if (!rtc.begin()) {
      RTC_DEBUG_PRINTLN("[RTC TASK] DS3231 not found");
      setRtcValid(false);
      rtcReady = false;

      while (1) {
        RTC_DEBUG_PRINTLN("[RTC TASK] waiting... RTC init failed");
        vTaskDelay(pdMS_TO_TICKS(3000));
      }
    }

    rtcReady = true;

    // ===== 컴파일 시간으로 RTC 초기화 =====
    DateTime compileTime(F(__DATE__), F(__TIME__));
    rtc.adjust(compileTime);

    RTC_DEBUG_PRINTF(
      "[RTC INIT] set from compile time: %04u-%02u-%02u %02u:%02u:%02u\n",
      compileTime.year(),
      compileTime.month(),
      compileTime.day(),
      compileTime.hour(),
      compileTime.minute(),
      compileTime.second()
    );

    if (rtc.lostPower()) {
      RTC_DEBUG_PRINTLN("[RTC TASK] WARNING: DS3231 lost power");
      RTC_DEBUG_PRINTLN("[RTC TASK] time may be invalid");
    }

    setRtcValid(true);
    RTC_DEBUG_PRINTLN("[RTC TASK] DS3231 init OK");

    uint8_t lastPrintedSecond = 255;

    while (1) {
      DateTime now = rtc.now();
      ClockData current = toClockData(now);

      setCurrentTime(current);
      setRtcValid(true);

      // ===== 시간 소스 결정 =====
      if (getGpsLocked()) {
        setTimeSource(TIME_SOURCE_GPS);
      } else {
        setTimeSource(TIME_SOURCE_RTC);
      }

      if (current.second != lastPrintedSecond) {
        lastPrintedSecond = current.second;
        RTC_DEBUG_PRINTF(
          "[RTC TASK] %04u-%02u-%02u %02u:%02u:%02u\n",
          current.year,
          current.month,
          current.day,
          current.hour,
          current.minute,
          current.second
        );
      }

      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

bool syncRTCFromGpsUtc(
  uint16_t year,
  uint8_t month,
  uint8_t day,
  uint8_t hour,
  uint8_t minute,
  uint8_t second
) {
  if (!rtcReady) {
    return false;
  }

  // UTC -> KST (UTC+9)
  addHours(year, month, day, hour, 9);

  DateTime target(year, month, day, hour, minute, second);
  rtc.adjust(target);

  RTC_DEBUG_PRINTF(
    "[RTC SYNC] adjusted from GPS UTC -> KST: %04u-%02u-%02u %02u:%02u:%02u\n",
    year, month, day, hour, minute, second
  );

  return true;
}

void startRTCTask() {
  xTaskCreatePinnedToCore(
    rtcTask,
    "TaskRTC",
    4096,
    nullptr,
    1,
    &rtcTaskHandle,
    1
  );
}