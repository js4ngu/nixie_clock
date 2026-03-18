#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <Arduino.h>
#include <Print.h>

void initDebugLog();
void appendDebugLog(const char* text);
void appendDebugLog(const String& text);
void getDebugLogSnapshot(String& out);
void debugLogPrintf(const char* format, ...);
void appendRtcLog(const char* text);
void appendRtcLog(const String& text);
void getRtcLogSnapshot(String& out);
void rtcLogPrintf(const char* format, ...);

class DebugStringPrint : public Print {
public:
  String data;

  size_t write(uint8_t c) override {
    data += static_cast<char>(c);
    return 1;
  }
};

template <typename T>
inline void debugLogPrint(const T& value) {
  DebugStringPrint buffer;
  Serial.print(value);
  buffer.print(value);
  appendDebugLog(buffer.data);
}

inline void debugLogPrintln() {
  Serial.println();
  appendDebugLog("\r\n");
}

template <typename T>
inline void debugLogPrintln(const T& value) {
  DebugStringPrint buffer;
  Serial.println(value);
  buffer.println(value);
  appendDebugLog(buffer.data);
}

template <typename T>
inline void rtcLogPrint(const T& value) {
  DebugStringPrint buffer;
  Serial.print(value);
  buffer.print(value);
  appendDebugLog(buffer.data);
  appendRtcLog(buffer.data);
}

inline void rtcLogPrintln() {
  Serial.println();
  appendDebugLog("\r\n");
  appendRtcLog("\r\n");
}

template <typename T>
inline void rtcLogPrintln(const T& value) {
  DebugStringPrint buffer;
  Serial.println(value);
  buffer.println(value);
  appendDebugLog(buffer.data);
  appendRtcLog(buffer.data);
}

#endif
