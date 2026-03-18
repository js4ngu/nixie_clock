#include "display_task.h"

#include <Arduino.h>

#include "app_state.h"
#include "debug_config.h"

namespace {
  TaskHandle_t displayTaskHandle = nullptr;

  constexpr int DATA   = 32;
  constexpr int DATA2  = 33;
  constexpr int CLK    = 25;
  constexpr int LATCH  = 14;
  constexpr int OE     = 26;
  constexpr int CLR    = 27;

  static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  constexpr uint32_t DISPLAY_REFRESH_MS = 100;
  constexpr uint32_t DIVERGENCE_FRAME_MS = 80;

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

  void writeRandomDigits() {
    uint8_t d1 = static_cast<uint8_t>(random(0, 10));
    uint8_t d2 = static_cast<uint8_t>(random(0, 10));
    uint8_t d3 = static_cast<uint8_t>(random(0, 10));
    uint8_t d4 = static_cast<uint8_t>(random(0, 10));
    writeDigits(d1, d2, d3, d4);
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

    randomSeed(micros());

    uint32_t nextDivergenceStartMs = millis() + (getDivergencePeriod() * 1000UL);
    uint32_t divergenceEndMs = 0;
    uint32_t nextDivergenceFrameMs = 0;
    bool divergenceActive = false;

    while (1) {
      AppState state;
      getAppStateSnapshot(state);

      const uint32_t nowMs = millis();
      const uint32_t divergencePeriodMs = state.divergencePeriod * 1000UL;

      if (!divergenceActive && divergencePeriodMs > 0 &&
          static_cast<int32_t>(nowMs - nextDivergenceStartMs) >= 0) {
        divergenceActive = true;
        divergenceEndMs = nowMs + state.divergenceEffectMs;
        nextDivergenceFrameMs = nowMs;
        nextDivergenceStartMs = nowMs + divergencePeriodMs;
        DISPLAY_DEBUG_PRINTF(
          "[DISPLAY] divergence effect started for %lu ms\n",
          static_cast<unsigned long>(state.divergenceEffectMs)
        );
      }

      if (divergenceActive) {
        if (static_cast<int32_t>(nowMs - nextDivergenceFrameMs) >= 0) {
          writeRandomDigits();
          nextDivergenceFrameMs = nowMs + DIVERGENCE_FRAME_MS;
        }

        if (static_cast<int32_t>(nowMs - divergenceEndMs) >= 0) {
          divergenceActive = false;
          DISPLAY_DEBUG_PRINTLN("[DISPLAY] divergence effect ended");
        }
      } else {
        uint8_t hour = state.currentTime.hour;
        uint8_t minute = state.currentTime.minute;
        uint8_t d1 = hour / 10;
        uint8_t d2 = hour % 10;
        uint8_t d3 = minute / 10;
        uint8_t d4 = minute % 10;

        writeDigits(d1, d2, d3, d4);
      }

      vTaskDelay(pdMS_TO_TICKS(DISPLAY_REFRESH_MS));
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
