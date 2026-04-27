// =============================================================================
// ph_watchdog.cpp – Watchdog matériel, gestion des échecs, deep-sleep sécurisé
// =============================================================================
#include "ph_globals.h"

// forward
void modem_off();

// -------------------------------------------------------------------------------------------------
// Bluetooth (inutilisé sur Physalia – désactivé pour économiser RAM et courant)
// -------------------------------------------------------------------------------------------------
void disableUnusedBluetooth()
{
#if defined(ARDUINO_ARCH_ESP32) && __has_include("esp32-hal-bt.h")
  if (btStarted()) {
    if (btStop()) { LOGLN(1, "Bluetooth stopped"); }
    else          { LOGLN(1, "Bluetooth stop failed"); }
  } else {
    LOGLN(1, "Bluetooth already stopped");
  }
#endif
}

// -------------------------------------------------------------------------------------------------
// Task Watchdog Timer (TWDT) – actif uniquement en mode deep-sleep
// -------------------------------------------------------------------------------------------------
void setupTaskWatchdog()
{
  if (!DEEP_SLEEP_ACTIVATED) return;

  esp_err_t err;
#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)
  esp_task_wdt_config_t cfg = {};
  cfg.timeout_ms     = TASK_WDT_TIMEOUT_S * 1000UL;
  cfg.idle_core_mask = 0;
  cfg.trigger_panic  = true;
  err = esp_task_wdt_init(&cfg);
  if (err == ESP_ERR_INVALID_STATE) err = esp_task_wdt_reconfigure(&cfg);
#else
  err = esp_task_wdt_init(TASK_WDT_TIMEOUT_S, true);
#endif

  if (err != ESP_OK) { LOGF(1, "TWDT init failed: %d\n", (int)err); return; }

  taskWdtEnabled = true;
  esp_task_wdt_add(nullptr);
  LOGLN(1, "TWDT enabled (60 s) – loopTask subscribed");
}

void registerCurrentTaskToWatchdog(const char *taskName)
{
  if (!taskWdtEnabled) return;
  const esp_err_t err = esp_task_wdt_add(nullptr);
  if (err == ESP_OK) {
    LOGF(1, "TWDT subscribed: %s\n", taskName ? taskName : "current");
  } else if (err != ESP_ERR_INVALID_ARG) {
    LOGF(1, "TWDT subscribe failed (%s): %d\n", taskName ? taskName : "?", (int)err);
  }
}

void feedTaskWatchdog()
{
  if (taskWdtEnabled) (void)esp_task_wdt_reset();
}

// -------------------------------------------------------------------------------------------------
// Compteur d'échecs
// -------------------------------------------------------------------------------------------------
void resetFailureCycleCounter(const char *reason)
{
  if (consecutiveFailureCount == 0) return;
  LOGF(1, "Failure counter reset (%s): %u → 0\n",
       reason ? reason : "success", (unsigned)consecutiveFailureCount);
  consecutiveFailureCount = 0;
}

// -------------------------------------------------------------------------------------------------
// safeDeepSleep() – point UNIQUE d'entrée en sommeil
//
// Garantit l'extinction du modem même si AT+CPOF ne répond pas (voir modem_off()).
// Toujours appeler cette fonction au lieu de (modem_off + delay + esp_deep_sleep_start).
// -------------------------------------------------------------------------------------------------
void safeDeepSleep()
{
  if (!DEEP_SLEEP_ACTIVATED) return;
  feedTaskWatchdog();
  modem_off();

  // Coupure UM980 via Pololu/MOSFET avant sommeil : évite une fuite GNSS hors acquisition.
  pinMode(PIN_GNSS_EN, OUTPUT);
  digitalWrite(PIN_GNSS_EN, LOW);

  feedTaskWatchdog();
  delay(1000);
  feedTaskWatchdog();
  esp_deep_sleep_start();
}

// -------------------------------------------------------------------------------------------------
// handleFailureCycleAndSleep() – gestion des cycles d'échec
// -------------------------------------------------------------------------------------------------
void handleFailureCycleAndSleep(const char *stage)
{
  ++consecutiveFailureCount;
  ++failureSleepCount;
  LOGF(1, "Failure at '%s': %u/%u (total failure sleeps: %u)\n",
       stage ? stage : "?",
       (unsigned)consecutiveFailureCount,
       (unsigned)FAILURE_REBOOT_THRESHOLD,
       (unsigned)failureSleepCount);

  if (consecutiveFailureCount >= FAILURE_REBOOT_THRESHOLD) {
    consecutiveFailureCount = 0;
    LOGF(1, "Threshold reached – sleeping to recover (failure sleeps: %u)\n",
         (unsigned)failureSleepCount);

    if (failureSleepCount >= (uint8_t)(FAILURE_REBOOT_THRESHOLD * 3)) {
      LOGLN(1, "Persistent failure – ESP.restart() to reset modem state");
      failureSleepCount   = 0;
      lastBootWasRestart  = true;
      lastRestartTimestampMs = (uint32_t)millis();
      delay(100);
      ESP.restart();
    }
  }

  safeDeepSleep();
}
