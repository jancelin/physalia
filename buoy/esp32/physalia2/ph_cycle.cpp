// =============================================================================
// ph_cycle.cpp – Stockage local des positions et détection d'état RTK
// =============================================================================
#include "ph_globals.h"

int mapPositionTypeToFix(const String &t);
int mapPositionTypeToCarrier(const String &t);

bool isRtkFixedType(const String &type)
{
  return type.indexOf("INT") >= 0;
}

bool isRtkFloatType(const String &type)
{
  return type.indexOf("FLOAT") >= 0;
}

void appendFixSample(const PvtslnData &fix)
{
  if (!fix.valid) return;

  if (fixBatchCount >= PH_FIX_BATCH_MAX_RECORDS) {
    // Conservation chronologique : on perd le plus ancien, jamais le plus récent.
    for (uint16_t i = 1; i < PH_FIX_BATCH_MAX_RECORDS; i++) {
      fixBatch[i - 1] = fixBatch[i];
    }
    fixBatch[PH_FIX_BATCH_MAX_RECORDS - 1] = fix;
    fixBatchDropped++;
  } else {
    fixBatch[fixBatchCount++] = fix;
  }

  if (isRtkFloatType(fix.bestposType) && firstFloat_ms == 0) {
    firstFloat_ms = millis();
    LOGF(1, "[RTK] First FLOAT at %lu ms: %s\n",
         (unsigned long)firstFloat_ms, fix.bestposType.c_str());
  }

  if (isRtkFixedType(fix.bestposType)) {
    const unsigned long now = millis();
    if (firstFix_ms == 0) {
      firstFix_ms = now;
      LOGF(1, "[RTK] First FIX at %lu ms: %s\n",
           (unsigned long)firstFix_ms, fix.bestposType.c_str());
    }
    if (rtkFixStart_ms == 0) {
      rtkFixStart_ms = now;
      lastState = now;  // compatibilité avec les traces historiques
      LOGF(1, "[RTK] Acquisition window starts after FIX at %lu ms\n",
           (unsigned long)rtkFixStart_ms);
    }
  }
}
