#ifndef DEBUG_CONFIG_H
#define DEBUG_CONFIG_H

#include <Arduino.h>
#include "debug_log.h"

// =========================
// Task debug enable switches
// 1 = enable, 0 = disable
// =========================
#define DEBUG_GPS_TASK         1
#define DEBUG_RTC_TASK         1
#define DEBUG_DISPLAY_TASK     0
#define DEBUG_WEB_SERVER_TASK  0
#define DEBUG_WEB_API          0

// =========================
// GPS task debug macros
// =========================
#if DEBUG_GPS_TASK
  #define GPS_DEBUG_PRINT(...)        debugLogPrint(__VA_ARGS__)
  #define GPS_DEBUG_PRINTLN(...)      debugLogPrintln(__VA_ARGS__)
  #define GPS_DEBUG_PRINTF(...)       debugLogPrintf(__VA_ARGS__)
#else
  #define GPS_DEBUG_PRINT(...)
  #define GPS_DEBUG_PRINTLN(...)
  #define GPS_DEBUG_PRINTF(...)
#endif

// =========================
// RTC task debug macros
// =========================
#if DEBUG_RTC_TASK
  #define RTC_DEBUG_PRINT(...)        rtcLogPrint(__VA_ARGS__)
  #define RTC_DEBUG_PRINTLN(...)      rtcLogPrintln(__VA_ARGS__)
  #define RTC_DEBUG_PRINTF(...)       rtcLogPrintf(__VA_ARGS__)
#else
  #define RTC_DEBUG_PRINT(...)
  #define RTC_DEBUG_PRINTLN(...)
  #define RTC_DEBUG_PRINTF(...)
#endif

// =========================
// Display task debug macros
// =========================
#if DEBUG_DISPLAY_TASK
  #define DISPLAY_DEBUG_PRINT(...)        debugLogPrint(__VA_ARGS__)
  #define DISPLAY_DEBUG_PRINTLN(...)      debugLogPrintln(__VA_ARGS__)
  #define DISPLAY_DEBUG_PRINTF(...)       debugLogPrintf(__VA_ARGS__)
#else
  #define DISPLAY_DEBUG_PRINT(...)
  #define DISPLAY_DEBUG_PRINTLN(...)
  #define DISPLAY_DEBUG_PRINTF(...)
#endif

// =========================
// Web server task debug macros
// =========================
#if DEBUG_WEB_SERVER_TASK
  #define WEB_TASK_DEBUG_PRINT(...)       debugLogPrint(__VA_ARGS__)
  #define WEB_TASK_DEBUG_PRINTLN(...)     debugLogPrintln(__VA_ARGS__)
  #define WEB_TASK_DEBUG_PRINTF(...)      debugLogPrintf(__VA_ARGS__)
#else
  #define WEB_TASK_DEBUG_PRINT(...)
  #define WEB_TASK_DEBUG_PRINTLN(...)
  #define WEB_TASK_DEBUG_PRINTF(...)
#endif

// =========================
// Web API debug macros
// =========================
#if DEBUG_WEB_API
  #define WEB_API_DEBUG_PRINT(...)        debugLogPrint(__VA_ARGS__)
  #define WEB_API_DEBUG_PRINTLN(...)      debugLogPrintln(__VA_ARGS__)
  #define WEB_API_DEBUG_PRINTF(...)       debugLogPrintf(__VA_ARGS__)
#else
  #define WEB_API_DEBUG_PRINT(...)
  #define WEB_API_DEBUG_PRINTLN(...)
  #define WEB_API_DEBUG_PRINTF(...)
#endif

#endif
