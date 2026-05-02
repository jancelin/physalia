// =============================================================================
// ph_cycle.cpp – Stockage local des positions et détection d'état RTK
// =============================================================================
// PATCH P1 — signatures adaptées de String& → const char*
// isRtkFixedType / isRtkFloatType utilisent strstr() sur char[] au lieu de
// String::indexOf(). Pas de surcharge String conservée : les deux seuls
// appelants (appendFixSample ici, et ph_mqtt.cpp) passent maintenant le
// tableau char[] directement.
// =============================================================================
#include "ph_globals.h"

int mapPositionTypeToFix(const char *t);
int mapPositionTypeToCarrier(const char *t);

// [P1] const char* — strstr() remplace String::indexOf()
bool isRtkFixedType(const char *type)
{
  return type && (strstr(type, "INT") != nullptr);
}

bool isRtkFloatType(const char *type)
{
  return type && (strstr(type, "FLOAT") != nullptr);
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

  // [P1] fix.bestposType est maintenant char[] → appel direct sans .c_str()
  if (isRtkFloatType(fix.bestposType) && firstFloat_ms == 0) {
    firstFloat_ms = millis();
    LOGF(1, "[RTK] First FLOAT at %lu ms: %s\n",
         (unsigned long)firstFloat_ms, fix.bestposType);
  }

  if (isRtkFixedType(fix.bestposType)) {
    const unsigned long now = millis();
    if (firstFix_ms == 0) {
      firstFix_ms = now;
      LOGF(1, "[RTK] First FIX at %lu ms: %s\n",
           (unsigned long)firstFix_ms, fix.bestposType);
    }
    if (rtkFixStart_ms == 0) {
      rtkFixStart_ms = now;
      lastState = now;
      LOGF(1, "[RTK] Acquisition window starts after FIX at %lu ms\n",
           (unsigned long)rtkFixStart_ms);
    }
  }
}
