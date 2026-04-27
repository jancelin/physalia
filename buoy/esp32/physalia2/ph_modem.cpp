// =============================================================================
// ph_modem.cpp – Gestion du modem SIM7600 (init, réseau, reset périodique)
// =============================================================================
#include "ph_globals.h"

// forward
void handleFailureCycleAndSleep(const char *stage);
void feedTaskWatchdog();

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
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  pinMode(LED_PIN,   OUTPUT); digitalWrite(LED_PIN,   HIGH);
  pinMode(POWER_PIN, OUTPUT); digitalWrite(POWER_PIN, HIGH);
  pinMode(PWR_PIN,   OUTPUT);
  digitalWrite(PWR_PIN, HIGH); delay(500);
  digitalWrite(PWR_PIN, LOW);

  LOGLN(1, "Initializing modem...");
  feedTaskWatchdog();
  if (!modem.testAT()) {
    LOGLN(1, "modem.testAT() failed – continuing anyway");
  }

  LOG(1, "Waiting for network...");
  unsigned long start = millis();
  // BUG CORRIGÉ : ACQUISION_PERIOD_4G est en secondes, millis() en ms → * 1000UL
  while (!modem.waitForNetwork(10000L) &&
         (millis() - start < (unsigned long)ACQUISION_PERIOD_4G * 1000UL)) {
    feedTaskWatchdog();
    LOGLN(1, "fail to find network, retrying in 10 s");
    delay(10000);
    feedTaskWatchdog();
  }

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
    unsigned long start = millis();
    // BUG CORRIGÉ : * 1000UL
    while (!modem.waitForNetwork(10000L) &&
           (millis() - start < (unsigned long)ACQUISION_PERIOD_4G * 1000UL)) {
      feedTaskWatchdog();
      LOGLN(1, "LOOP - fail to find network, retrying in 10 s");
      delay(10000);
      feedTaskWatchdog();
    }

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
