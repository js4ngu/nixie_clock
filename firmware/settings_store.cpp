#include "settings_store.h"

#include <Preferences.h>
#include "app_state.h"

namespace {
  Preferences prefs;
  const char* NS_NAME = "nixie";
  const char* KEY_DIMMING = "dimming";
  const char* KEY_PERIOD = "period";
}

void loadSettings() {
  prefs.begin(NS_NAME, true);

  uint8_t dimming = prefs.getUChar(KEY_DIMMING, 50);
  uint32_t period = prefs.getUInt(KEY_PERIOD, 3600);

  prefs.end();

  setDimming(dimming);
  setDivergencePeriod(period);

  Serial.printf("[SETTINGS] loaded: dimming=%u, divergencePeriod=%lu sec\n",
                dimming, period);
}

void saveDimmingSetting(uint8_t dimming) {
  prefs.begin(NS_NAME, false);
  prefs.putUChar(KEY_DIMMING, dimming);
  prefs.end();

  Serial.printf("[SETTINGS] saved dimming=%u\n", dimming);
}

void saveDivergencePeriodSetting(uint32_t periodSec) {
  prefs.begin(NS_NAME, false);
  prefs.putUInt(KEY_PERIOD, periodSec);
  prefs.end();

  Serial.printf("[SETTINGS] saved divergencePeriod=%lu sec\n", periodSec);
}