#include "display_task.h"
#include <Arduino.h>
#include "app_state.h"

namespace {

  TaskHandle_t displayTaskHandle = nullptr;

  constexpr int DATA   = 32;
  constexpr int DATA2  = 33;
  constexpr int CLK    = 25;
  constexpr int LATCH  = 14;
  constexpr int OE     = 26;
  constexpr int CLR    = 27;

  static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

  // ===== digit → bit 변환 (일단 그대로) =====
  uint8_t digitToByte(uint8_t d) {
    return d & 0x0F;  // 이후 mapping 테이블로 교체
  }

  // ===== 안전한 출력 =====
  void writeDigits(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4) {
    portENTER_CRITICAL(&mux);

    digitalWrite(LATCH, LOW);

    // DATA2 (뒤쪽 두 자리)
    shiftOut(DATA2, CLK, MSBFIRST, digitToByte(d3));
    shiftOut(DATA2, CLK, MSBFIRST, digitToByte(d4));

    // DATA (앞쪽 두 자리)
    shiftOut(DATA, CLK, MSBFIRST, digitToByte(d1));
    shiftOut(DATA, CLK, MSBFIRST, digitToByte(d2));

    digitalWrite(LATCH, HIGH);

    portEXIT_CRITICAL(&mux);
  }

  void displayTask(void *pvParameters) {
    (void)pvParameters;

    pinMode(DATA, OUTPUT);
    pinMode(DATA2, OUTPUT);
    pinMode(CLK, OUTPUT);
    pinMode(LATCH, OUTPUT);
    pinMode(OE, OUTPUT);
    pinMode(CLR, OUTPUT);

    digitalWrite(OE, LOW);
    digitalWrite(CLR, HIGH);

    while (1) {

      // ===== AppState에서 시간 읽기 =====
      AppState state;
      getAppStateSnapshot(state);  // :contentReference[oaicite:0]{index=0}

      uint8_t hour   = state.currentTime.hour;
      uint8_t minute = state.currentTime.minute;

      // ===== 자리 분리 =====
      uint8_t d1 = hour / 10;
      uint8_t d2 = hour % 10;
      uint8_t d3 = minute / 10;
      uint8_t d4 = minute % 10;

      // ===== 출력 =====
      writeDigits(d1, d2, d3, d4);

      Serial.printf(
        "[DISPLAY] %02u:%02u -> %u %u %u %u\n",
        hour, minute, d1, d2, d3, d4
      );

      vTaskDelay(pdMS_TO_TICKS(100));  // 100ms 추천
    }
  }
}

void startDisplayTask() {
  xTaskCreatePinnedToCore(
    displayTask,
    "TaskDisplay",
    4096,
    nullptr,
    2,
    &displayTaskHandle,
    1
  );
}