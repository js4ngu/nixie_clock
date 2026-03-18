#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H

#include <Arduino.h>

void loadSettings();
void saveDimmingSetting(uint8_t dimming);
void saveDivergencePeriodSetting(uint32_t periodSec);

#endif