// =============================================================================
// ph_sleep.cpp – Deep-sleep, wakeup reason, réinitialisation RTC au boot froid
// =============================================================================
#include "ph_globals.h"

// forwards
void feedTaskWatchdog();

// -------------------------------------------------------------------------------------------------
// setupDeepSleep() – configure le timer de réveil
// -------------------------------------------------------------------------------------------------
void setupDeepSleep()
{
  LOGF(1, "DeepSleep: %s | sleep=%ds | acq=%ds\n",
       DEEP_SLEEP_ACTIVATED ? "ON" : "OFF",
       TIME_TO_SLEEP, RTK_ACQUISITION_PERIOD);

  if (DEEP_SLEEP_ACTIVATED) {
    esp_sleep_enable_timer_wakeup(
        (uint64_t)TIME_TO_SLEEP * uS_TO_S_FACTOR);
  }
}

// -------------------------------------------------------------------------------------------------
// print_wakeup_reason() – log la cause du réveil ; réinitialise la batterie au boot froid
// -------------------------------------------------------------------------------------------------
void print_wakeup_reason()
{
  LOGLN(1, "-----------------");
  LOGLN(1, " - WAKEUP REASON ");
  ++bootCount;
  LOGF(1, " - Boot #%u  |  failure sleeps: %u\n",
       (unsigned)bootCount, (unsigned)failureSleepCount);

  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT0:
      LOGLN(1, "Réveil EXT0 (RTC_IO)");         break;
    case ESP_SLEEP_WAKEUP_EXT1:
      LOGLN(1, "Réveil EXT1 (RTC_CNTL)");       break;
    case ESP_SLEEP_WAKEUP_TIMER:
      LOGLN(1, "Réveil timer");                  break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      LOGLN(1, "Réveil touchpad");               break;
    default:
      LOGLN(1, "Boot froid (power-on / reset)");
      // Réinitialise l'historique batterie à chaque boot froid
      rtcVbatMinMv   = 9999;
      rtcVbatEmaMv   = 0;
      rtcVbatEmaInit = false;
      break;
  }
}
