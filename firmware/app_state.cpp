#include "app_state.h"

AppState gAppState = {
  false,                // gpsLocked
  false,                // gpsReceiving
  false,                // gpsEverLocked
  false,                // rtcValid
  0,                    // gpsSatellites
  50,                   // dimming
  3600,                 // divergencePeriod
  TIME_SOURCE_RTC,      // timeSource
  {2025, 1, 1, 0, 0, 0} // currentTime
};

SemaphoreHandle_t gAppStateMutex = nullptr;

void initAppState() {
  gAppStateMutex = xSemaphoreCreateMutex();
}

void setGpsLocked(bool locked) {
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  gAppState.gpsLocked = locked;
  if (locked) {
    gAppState.gpsEverLocked = true;
  }
  xSemaphoreGive(gAppStateMutex);
}

bool getGpsLocked() {
  bool locked;
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  locked = gAppState.gpsLocked;
  xSemaphoreGive(gAppStateMutex);
  return locked;
}

void setGpsReceiving(bool receiving) {
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  gAppState.gpsReceiving = receiving;
  xSemaphoreGive(gAppStateMutex);
}

bool getGpsReceiving() {
  bool receiving;
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  receiving = gAppState.gpsReceiving;
  xSemaphoreGive(gAppStateMutex);
  return receiving;
}

void setGpsEverLocked(bool locked) {
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  gAppState.gpsEverLocked = locked;
  xSemaphoreGive(gAppStateMutex);
}

bool getGpsEverLocked() {
  bool locked;
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  locked = gAppState.gpsEverLocked;
  xSemaphoreGive(gAppStateMutex);
  return locked;
}

void setGpsSatellites(uint8_t sats) {
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  gAppState.gpsSatellites = sats;
  xSemaphoreGive(gAppStateMutex);
}

uint8_t getGpsSatellites() {
  uint8_t sats;
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  sats = gAppState.gpsSatellites;
  xSemaphoreGive(gAppStateMutex);
  return sats;
}

void setTimeSource(TimeSource source) {
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  gAppState.timeSource = source;
  xSemaphoreGive(gAppStateMutex);
}

TimeSource getTimeSource() {
  TimeSource source;
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  source = gAppState.timeSource;
  xSemaphoreGive(gAppStateMutex);
  return source;
}

void setDimming(uint8_t value) {
  if (value > 100) value = 100;

  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  gAppState.dimming = value;
  xSemaphoreGive(gAppStateMutex);
}

uint8_t getDimming() {
  uint8_t value;
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  value = gAppState.dimming;
  xSemaphoreGive(gAppStateMutex);
  return value;
}

void setDivergencePeriod(uint32_t value) {
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  gAppState.divergencePeriod = value;
  xSemaphoreGive(gAppStateMutex);
}

uint32_t getDivergencePeriod() {
  uint32_t value;
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  value = gAppState.divergencePeriod;
  xSemaphoreGive(gAppStateMutex);
  return value;
}

void setRtcValid(bool valid) {
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  gAppState.rtcValid = valid;
  xSemaphoreGive(gAppStateMutex);
}

bool getRtcValid() {
  bool valid;
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  valid = gAppState.rtcValid;
  xSemaphoreGive(gAppStateMutex);
  return valid;
}

void setCurrentTime(const ClockData& timeData) {
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  gAppState.currentTime = timeData;
  xSemaphoreGive(gAppStateMutex);
}

ClockData getCurrentTime() {
  ClockData out;
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  out = gAppState.currentTime;
  xSemaphoreGive(gAppStateMutex);
  return out;
}

void getAppStateSnapshot(AppState &outState) {
  xSemaphoreTake(gAppStateMutex, portMAX_DELAY);
  outState = gAppState;
  xSemaphoreGive(gAppStateMutex);
}