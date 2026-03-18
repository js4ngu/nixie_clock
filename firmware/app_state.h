#ifndef APP_STATE_H
#define APP_STATE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct ClockData {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

enum TimeSource {
  TIME_SOURCE_RTC = 0,
  TIME_SOURCE_GPS = 1
};

struct AppState {
  bool gpsLocked;
  bool gpsReceiving;
  bool gpsEverLocked;
  bool rtcValid;
  uint8_t gpsSatellites;
  uint8_t dimming;
  uint32_t divergencePeriod;
  TimeSource timeSource;
  ClockData currentTime;
};

extern AppState gAppState;
extern SemaphoreHandle_t gAppStateMutex;

void initAppState();

void setGpsLocked(bool locked);
bool getGpsLocked();

void setGpsReceiving(bool receiving);
bool getGpsReceiving();

void setGpsEverLocked(bool locked);
bool getGpsEverLocked();

void setGpsSatellites(uint8_t sats);
uint8_t getGpsSatellites();

void setTimeSource(TimeSource source);
TimeSource getTimeSource();

void setDimming(uint8_t value);
uint8_t getDimming();

void setDivergencePeriod(uint32_t value);
uint32_t getDivergencePeriod();

void setRtcValid(bool valid);
bool getRtcValid();

void setCurrentTime(const ClockData& timeData);
ClockData getCurrentTime();

void getAppStateSnapshot(AppState &outState);

#endif