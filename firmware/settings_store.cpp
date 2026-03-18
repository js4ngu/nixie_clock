#include "settings_store.h"

#include <Preferences.h>
#include "app_state.h"
#include "debug_log.h"

namespace {
  Preferences prefs;
  const char* NS_NAME = "nixie";
  const char* KEY_DIMMING = "dimming";
  const char* KEY_PERIOD = "period";
  const char* KEY_EFFECT = "effect_ms";
}

void loadSettings() {
  prefs.begin(NS_NAME, true);

  uint8_t dimming = prefs.getUChar(KEY_DIMMING, 50);
  uint32_t period = prefs.getUInt(KEY_PERIOD, 3600);
  uint16_t effectMs = prefs.getUShort(KEY_EFFECT, 2500);

  prefs.end();

  setDimming(dimming);
  setDivergencePeriod(period);
  setDivergenceEffectMs(effectMs);

  debugLogPrintf(
    "[SETTINGS] loaded: dimming=%u, divergencePeriod=%lu sec, divergenceEffect=%u ms\n",
    dimming,
    period,
    effectMs
  );
}

void saveDimmingSetting(uint8_t dimming) {
  prefs.begin(NS_NAME, false);
  prefs.putUChar(KEY_DIMMING, dimming);
  prefs.end();

  debugLogPrintf("[SETTINGS] saved dimming=%u\n", dimming);
}

void saveDivergencePeriodSetting(uint32_t periodSec) {
  prefs.begin(NS_NAME, false);
  prefs.putUInt(KEY_PERIOD, periodSec);
  prefs.end();

  debugLogPrintf("[SETTINGS] saved divergencePeriod=%lu sec\n", periodSec);
}

void saveDivergenceEffectSetting(uint16_t effectMs) {
  prefs.begin(NS_NAME, false);
  prefs.putUShort(KEY_EFFECT, effectMs);
  prefs.end();

  debugLogPrintf("[SETTINGS] saved divergenceEffect=%u ms\n", effectMs);
}
