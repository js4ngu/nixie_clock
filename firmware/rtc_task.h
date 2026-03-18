#ifndef RTC_TASK_H
#define RTC_TASK_H

#include <Arduino.h>

void startRTCTask();

// GPS UTC -> RTC(KST) 동기화
bool syncRTCFromGpsUtc(
  uint16_t year,
  uint8_t month,
  uint8_t day,
  uint8_t hour,
  uint8_t minute,
  uint8_t second
);

#endif