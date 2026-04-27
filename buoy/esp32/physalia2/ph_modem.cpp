// =============================================================================
// ph_modem.cpp – Gestion du modem SIM7600 (init, réseau, reset périodique)
// =============================================================================
#include "ph_globals.h"

// forward
void handleFailureCycleAndSleep(const char *stage);
void feedTaskWatchdog();

// État local : séquence de mise sous tension SIM7600 lancée tôt depuis setup().
static bool modemPowerStartedEarly = false;
static unsigned long modemPowerEarly_ms = 0;

// -------------------------------------------------------------------------------------------------
// modemPowerStartEarly() – démarrage matériel SIM7600 le plus tôt possible
// -------------------------------------------------------------------------------------------------
void modemPowerStartEarly()
{
  if (modemPowerStartedEarly) return;

  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);

  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  feedTaskWatchdog();
  delay(500);
  feedTaskWatchdog();
  digitalWrite(PWR_PIN, LOW);

  modemPowerStartedEarly = true;
  modemPowerEarly_ms = millis();
  LOGF(1, "SIM7600 early power sequence started at %lu ms\n",
       (unsigned long)modemPowerEarly_ms);
}

// -------------------------------------------------------------------------------------------------
// fastModemAtSync() - synchronisation AT courte, sans blocage 10 s
// -------------------------------------------------------------------------------------------------
static bool fastModemAtSync(uint32_t timeoutMs)
{
  const unsigned long start = millis();
  uint8_t attempts = 0;

  while ((millis() - start) < timeoutMs) {
    feedTaskWatchdog();
    attempts++;

    if (modem.testAT(PH_MODEM_AT_ATTEMPT_TIMEOUT_MS)) {
      LOGF(1, "Modem AT sync OK in %lu ms (%u attempts)\n",
           (unsigned long)(millis() - start), attempts);
      return true;
    }

    feedTaskWatchdog();
    delay(100);
  }

  LOGF(1, "Modem AT sync not ready after %lu ms - continue network attach\n",
       (unsigned long)(millis() - start));
  return false;
}

// -------------------------------------------------------------------------------------------------
// waitForNetworkEnergyAware() - attachement LTE par tranches courtes, sans pause fixe de 10 s
// -------------------------------------------------------------------------------------------------
static bool waitForNetworkEnergyAware(uint32_t totalTimeoutMs)
{
  const unsigned long start = millis();

  while ((millis() - start) < totalTimeoutMs) {
    feedTaskWatchdog();

    const unsigned long elapsed = millis() - start;
    const uint32_t remaining = (elapsed >= totalTimeoutMs) ? 0UL : (totalTimeoutMs - elapsed);
    const uint32_t slice = min((uint32_t)PH_MODEM_NETWORK_POLL_MS, remaining);
    if (slice == 0) break;

    if (modem.waitForNetwork(slice)) {
      LOGF(1, "Network connected in %lu ms after attach start\n",
           (unsigned long)(millis() - start));
      return true;
    }

    feedTaskWatchdog();
    delay(PH_MODEM_NETWORK_RETRY_DELAY_MS);
  }

  return modem.isNetworkConnected();
}

// -------------------------------------------------------------------------------------------------
// periodicModemHardReset() – reset hardware toutes les 24h
//
// Le SIM7600 peut accumuler des états corrompus après plusieurs jours :
//   - Sessions GPRS non libérées par le carrier (expire à ~72-96h)
//   - Buffers UART internes saturés
//   - Registration réseau incohérente après handover de cellule
//
// BUG CORRIGÉ (arrêt j+4) : sans ce reset, ces états persistaient jusqu'à
// défaillance complète. Appelé AVANT setupGsm() à chaque réveil.
// -------------------------------------------------------------------------------------------------
void periodicModemHardReset()
{
  ++cyclesSinceModemReset;
  if (cyclesSinceModemReset < PERIODIC_MODEM_RESET_CYCLES) {
    LOGF(2, "Modem reset in %lu cycles\n",
         (unsigned long)(PERIODIC_MODEM_RESET_CYCLES - cyclesSinceModemReset));
    return;
  }

  LOGF(1, "Periodic modem hard reset (cycle %lu)\n",
       (unsigned long)cyclesSinceModemReset);
  cyclesSinceModemReset = 0;

  // Power-off hardware : hold PWR_PIN LOW > 1.5 s (spec SIM7600)
  feedTaskWatchdog();
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  feedTaskWatchdog(); delay(1600); feedTaskWatchdog();
  digitalWrite(PWR_PIN, HIGH);
  feedTaskWatchdog(); delay(500); feedTaskWatchdog();

  // Coupure DC-DC boost (POWER_PIN) pendant 1 s
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, LOW);
  feedTaskWatchdog(); delay(1000); feedTaskWatchdog();
  digitalWrite(POWER_PIN, HIGH);
  feedTaskWatchdog(); delay(500); feedTaskWatchdog();

  LOGLN(1, "Periodic modem hard reset complete");
}

// -------------------------------------------------------------------------------------------------
// modem_on() – séquence d'allumage SIM7600
// -------------------------------------------------------------------------------------------------
void modem_on()
{
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  delay(10);
  digitalWrite(POWER_PIN, LOW);
  delay(1010);
  digitalWrite(POWER_PIN, HIGH);
  LOGLN(1, "Waiting till modem ready...");
  feedTaskWatchdog(); delay(4510); feedTaskWatchdog();
}

// -------------------------------------------------------------------------------------------------
// modem_off() – extinction garantie même si AT+CPOF ne répond pas
// -------------------------------------------------------------------------------------------------
void modem_off()
{
  LOGLN(1, "Going to sleep now with modem turned off");
  feedTaskWatchdog();

  modem.sleepEnable(false);  // best-effort
  feedTaskWatchdog();

  const unsigned long t0 = millis();
  modem.poweroff();          // AT+CPOF – peut bloquer si modem muet
  const unsigned long dur = millis() - t0;
  feedTaskWatchdog();

  if (dur > MODEM_POWEROFF_TIMEOUT_MS) {
    LOGF(1, "modem_off: AT+CPOF timeout (%lu ms) – hardware power cut\n", dur);
  }

  // Coupure hardware GARANTIE (PWR_PIN LOW > 1.5 s)
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  feedTaskWatchdog(); delay(1600); feedTaskWatchdog();
  digitalWrite(PWR_PIN, HIGH);
  feedTaskWatchdog(); delay(300); feedTaskWatchdog();
}

// -------------------------------------------------------------------------------------------------
// setupGsm() – initialisation modem + connexion 4G/GPRS au boot
// -------------------------------------------------------------------------------------------------
void setupGsm()
{
  delay(10);

  if (!modemPowerStartedEarly) {
    // Secours : conserve le comportement fonctionnel historique si setup()
    // n’a pas déjà lancé la séquence matérielle.
    modemPowerStartEarly();
  }

  LOGF(1, "Initializing modem (early fast attach, powered %lu ms ago)...\n",
       (unsigned long)(millis() - modemPowerEarly_ms));
  feedTaskWatchdog();

  // Ancien comportement : modem.testAT() utilisait le timeout par défaut (~10 s).
  // Sur le terrain, ce test échouait puis le réseau se connectait quand même.
  // On remplace donc ce blocage par une synchronisation AT courte puis un polling LTE.
  fastModemAtSync(PH_MODEM_AT_SYNC_TIMEOUT_MS);

  LOG(1, "Waiting for network...");
  waitForNetworkEnergyAware((uint32_t)ACQUISION_PERIOD_4G * 1000UL);

  if (!modem.isNetworkConnected()) {
    LOGLN(1, "Network connection failed");
    if (DEEP_SLEEP_ACTIVATED) handleFailureCycleAndSleep("4G");
  } else {
    LOGLN(1, "Network connected");
  }

#if TINY_GSM_USE_GPRS
  LOG(1, F("Connecting to APN: ")); LOG(1, apn);
  feedTaskWatchdog();
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    LOGLN(1, " fail");
    if (DEEP_SLEEP_ACTIVATED) handleFailureCycleAndSleep("GPRS");
  }
  LOGLN(1, " success");
  if (modem.isGprsConnected()) LOGLN(1, "GPRS connected");
#endif
}

// -------------------------------------------------------------------------------------------------
// maintainNetwork() – surveillance et reconnexion réseau dans loop()
// -------------------------------------------------------------------------------------------------
void maintainNetwork()
{
  if (!modem.isNetworkConnected()) {
    LOGLN(1, "LOOP - Network disconnected");
    // Reconnexion par tranches courtes, sans pause fixe de 10 s.
    waitForNetworkEnergyAware((uint32_t)ACQUISION_PERIOD_4G * 1000UL);

    if (!modem.isNetworkConnected()) {
      if (DEEP_SLEEP_ACTIVATED) {
        LOGLN(1, "LOOP - 4G timeout, deep sleep");
        handleFailureCycleAndSleep("4G");
      }
    } else {
      LOGLN(1, "LOOP - Network re-connected");
    }
  }

#if TINY_GSM_USE_GPRS
  if (!modem.isGprsConnected()) {
    LOGLN(1, "GPRS disconnected!");
    LOG(1, F("Connecting to ")); LOGLN(1, apn);
    feedTaskWatchdog();
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      LOGLN(1, "GPRS reconnect failed"); delay(10000); return;
    }
    if (modem.isGprsConnected()) LOGLN(1, "GPRS reconnected");
  }
#endif
}
