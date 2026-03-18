#include "debug_log.h"

#include <cstring>
#include <cstdarg>
#include <cstdio>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace {
  constexpr size_t DEBUG_LOG_CAPACITY = 8192;
  constexpr size_t RTC_LOG_CAPACITY = 4096;

  char gDebugLogBuffer[DEBUG_LOG_CAPACITY];
  size_t gDebugLogStart = 0;
  size_t gDebugLogLength = 0;
  SemaphoreHandle_t gDebugLogMutex = nullptr;

  char gRtcLogBuffer[RTC_LOG_CAPACITY];
  size_t gRtcLogStart = 0;
  size_t gRtcLogLength = 0;
  SemaphoreHandle_t gRtcLogMutex = nullptr;

  void appendLogBytes(
    char* buffer,
    size_t capacity,
    size_t& start,
    size_t& length,
    const char* text,
    size_t len
  ) {
    if (text == nullptr || len == 0 || capacity == 0) {
      return;
    }

    if (len >= capacity) {
      text += len - (capacity - 1);
      len = capacity - 1;
    }

    while ((length + len) >= capacity) {
      start = (start + 1) % capacity;
      length--;
    }

    for (size_t i = 0; i < len; ++i) {
      size_t index = (start + length) % capacity;
      buffer[index] = text[i];
      length++;
    }
  }

  void buildLogSnapshot(
    String& out,
    const char* buffer,
    size_t capacity,
    size_t start,
    size_t length
  ) {
    out = "";
    out.reserve(length);

    for (size_t i = 0; i < length; ++i) {
      size_t index = (start + i) % capacity;
      out += buffer[index];
    }
  }
}

void initDebugLog() {
  if (gDebugLogMutex == nullptr) {
    gDebugLogMutex = xSemaphoreCreateMutex();
  }

  if (gRtcLogMutex == nullptr) {
    gRtcLogMutex = xSemaphoreCreateMutex();
  }
}

void appendDebugLog(const char* text) {
  if (text == nullptr || gDebugLogMutex == nullptr) {
    return;
  }

  xSemaphoreTake(gDebugLogMutex, portMAX_DELAY);
  appendLogBytes(
    gDebugLogBuffer,
    DEBUG_LOG_CAPACITY,
    gDebugLogStart,
    gDebugLogLength,
    text,
    strlen(text)
  );
  xSemaphoreGive(gDebugLogMutex);
}

void appendDebugLog(const String& text) {
  appendDebugLog(text.c_str());
}

void getDebugLogSnapshot(String& out) {
  if (gDebugLogMutex == nullptr) {
    out = "";
    return;
  }

  xSemaphoreTake(gDebugLogMutex, portMAX_DELAY);
  buildLogSnapshot(out, gDebugLogBuffer, DEBUG_LOG_CAPACITY, gDebugLogStart, gDebugLogLength);
  xSemaphoreGive(gDebugLogMutex);
}

void debugLogPrintf(const char* format, ...) {
  if (format == nullptr) {
    return;
  }

  char stackBuffer[256];

  va_list args;
  va_start(args, format);
  int needed = vsnprintf(stackBuffer, sizeof(stackBuffer), format, args);
  va_end(args);

  if (needed < 0) {
    return;
  }

  if (needed < static_cast<int>(sizeof(stackBuffer))) {
    Serial.print(stackBuffer);
    appendDebugLog(stackBuffer);
    return;
  }

  size_t dynamicSize = static_cast<size_t>(needed) + 1;
  char* dynamicBuffer = new char[dynamicSize];
  if (dynamicBuffer == nullptr) {
    return;
  }

  va_start(args, format);
  vsnprintf(dynamicBuffer, dynamicSize, format, args);
  va_end(args);

  Serial.print(dynamicBuffer);
  appendDebugLog(dynamicBuffer);
  delete[] dynamicBuffer;
}

void appendRtcLog(const char* text) {
  if (text == nullptr || gRtcLogMutex == nullptr) {
    return;
  }

  xSemaphoreTake(gRtcLogMutex, portMAX_DELAY);
  appendLogBytes(
    gRtcLogBuffer,
    RTC_LOG_CAPACITY,
    gRtcLogStart,
    gRtcLogLength,
    text,
    strlen(text)
  );
  xSemaphoreGive(gRtcLogMutex);
}

void appendRtcLog(const String& text) {
  appendRtcLog(text.c_str());
}

void getRtcLogSnapshot(String& out) {
  if (gRtcLogMutex == nullptr) {
    out = "";
    return;
  }

  xSemaphoreTake(gRtcLogMutex, portMAX_DELAY);
  buildLogSnapshot(out, gRtcLogBuffer, RTC_LOG_CAPACITY, gRtcLogStart, gRtcLogLength);
  xSemaphoreGive(gRtcLogMutex);
}

void rtcLogPrintf(const char* format, ...) {
  if (format == nullptr) {
    return;
  }

  char stackBuffer[256];

  va_list args;
  va_start(args, format);
  int needed = vsnprintf(stackBuffer, sizeof(stackBuffer), format, args);
  va_end(args);

  if (needed < 0) {
    return;
  }

  if (needed < static_cast<int>(sizeof(stackBuffer))) {
    Serial.print(stackBuffer);
    appendDebugLog(stackBuffer);
    appendRtcLog(stackBuffer);
    return;
  }

  size_t dynamicSize = static_cast<size_t>(needed) + 1;
  char* dynamicBuffer = new char[dynamicSize];
  if (dynamicBuffer == nullptr) {
    return;
  }

  va_start(args, format);
  vsnprintf(dynamicBuffer, dynamicSize, format, args);
  va_end(args);

  Serial.print(dynamicBuffer);
  appendDebugLog(dynamicBuffer);
  appendRtcLog(dynamicBuffer);
  delete[] dynamicBuffer;
}
