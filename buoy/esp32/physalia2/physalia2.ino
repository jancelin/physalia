/*
=============================================================================================
PHYSALIA2 - UM980 edition
  - LTE NTRIP rover over SIM7600 (T-SIM7600G-H R2)
  - GNSS telemetry over MQTT
  - RTK fix via réseau Centipède-RTK (NTRIP)

Version énergie / mesure scientifique :
  Core 0 : gnssTask – lit l'UM980 en continu et pousse les PVTSLNA dans une queue courte.
  Core 1 : loop     – LTE/NTRIP/RTCM, stockage local des positions, publication MQTT groupée.
  
  Structure des fichiers :
  physalia2_dualcore.ino  – globals, setup(), loop(), gnssTask()
  ph_config.h             – pins, macros log, includes communs
  ph_globals.h            – struct PvtslnData + extern des globals
  ph_watchdog.cpp         – TWDT, failure handling, safeDeepSleep, BT off
  ph_modem.cpp            – setupGsm, maintainNetwork, modem_on/off, reset périodique
  ph_gnss.cpp             – processGnssSerial, parsePvtslnLine, setupGnss
  ph_ntrip.cpp            – maintainNtrip, processNtripStream, sendPeriodicGGA
  ph_mqtt.cpp             – publishFix, reconnect, callback, mapPositionType
  ph_battery.cpp          – ADC, readVbatMv, publishBatteryIfDue, publishPreSleepVbat
  ph_sleep.cpp            – setupDeepSleep, print_wakeup_reason
  secrets.h               – identifiants, paramètres réseau/timing
  secrets.cpp             – définitions uniques des variables globales
  NTRIPClient.h/.cpp      – librairie NTRIP
=============================================================================================
*/

#include "ph_config.h"
#include "ph_globals.h"
#include "secrets.h"

// =============================================================================
// DÉFINITIONS des objets et variables globales (extern déclarés dans ph_globals.h)
// =============================================================================

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger _dbg(Serial1, Serial);
  TinyGsm        modem(_dbg);
#else
  TinyGsm        modem(Serial1);
#endif

TinyGsmClient  mqttTransport(modem, 0);
TinyGsmClient  ntripTransport(modem, 2);
PubSubClient   mqtt(mqttTransport);
NTRIPClient    ntripClient(ntripTransport);
HardwareSerial GNSSSerial(2);

// --- Timing / état cycle ---
unsigned long bootStarted_ms      = 0;
unsigned long lastReconnectAttempt = 0;
unsigned long lastReceivedRTCM_ms  = 0;
unsigned long lastGgaToCaster_ms   = 0;
unsigned long lastPvtsln_ms        = 0;
unsigned long lastState            = 0;   // alias historique : début de fenêtre utile RTK FIX
unsigned long firstGga_ms          = 0;
unsigned long lastNtripAttempt_ms  = 0;
unsigned long ntripConnected_ms    = 0;
unsigned long firstRtcm_ms         = 0;
unsigned long firstFloat_ms        = 0;
unsigned long firstFix_ms          = 0;
unsigned long rtkFixStart_ms       = 0;
bool ggaSentToCaster               = false;
bool cycleClosed                   = false;

// --- Buffer UART GNSS ---
char   gnssLine[GNSS_LINE_BUFFER_SIZE];
size_t gnssLineLen = 0;
String lastGGASentence;

// --- Dernier fix PVTSLNA + batch de positions du cycle ---
PvtslnData lastFix;
PvtslnData fixBatch[PH_FIX_BATCH_MAX_RECORDS];
uint16_t fixBatchCount = 0;
uint32_t fixBatchDropped = 0;

// --- Batterie : batch local publié en fin de cycle ---
BatterySample batterySamples[PH_BATTERY_BATCH_MAX_RECORDS];
uint8_t batterySampleCount = 0;

// --- FreeRTOS ---
QueueHandle_t     pvtQueue  = nullptr;
SemaphoreHandle_t ggaMutex  = nullptr;

// --- Watchdog ---
bool taskWdtEnabled = false;

// --- RTC RAM (persist à travers deep-sleep) ---
RTC_DATA_ATTR uint32_t bootCount              = 0;
RTC_DATA_ATTR uint8_t  consecutiveFailureCount= 0;
RTC_DATA_ATTR uint32_t cyclesSinceModemReset  = 0;
RTC_DATA_ATTR uint8_t  failureSleepCount      = 0;
RTC_DATA_ATTR bool     lastBootWasRestart     = false;
RTC_DATA_ATTR uint32_t lastRestartTimestampMs = 0;
RTC_DATA_ATTR uint32_t cycleId                = 0;

// --- Batterie ---
int    vref         = 1100;
uint32_t timeStamp  = 0;
esp_adc_cal_characteristics_t adcChars;
bool   adcCalibrated = false;
RTC_DATA_ATTR int32_t rtcVbatMinMv   = 9999;
RTC_DATA_ATTR int32_t rtcVbatEmaMv   = 0;
RTC_DATA_ATTR bool    rtcVbatEmaInit = false;

// =============================================================================
// PROTOTYPES
// =============================================================================
void disableUnusedBluetooth();
void setupTaskWatchdog();
void registerCurrentTaskToWatchdog(const char *name);
void feedTaskWatchdog();
void resetFailureCycleCounter(const char *reason);
void handleFailureCycleAndSleep(const char *stage);
void safeDeepSleep();
void periodicModemHardReset();
void setupGsm();
void maintainNetwork();
void modem_on();
void modemPowerStartEarly();
void modem_off();
void setupGnss();
void processGnssSerial();
void processGnssLine(const char *line);
bool parsePvtslnLine(const char *line, PvtslnData &out);
void maintainNtrip();
void processNtripStream();
void sendPeriodicGGA();
void publishFix(const PvtslnData &fix);
boolean reconnect();
void callback(char *topic, byte *payload, unsigned int length);
void setupBatteryCalibration();
int32_t readVbatMv();
void publishBatteryIfDue(const char *dt);
void publishPreSleepVbat();
void recordBatterySample(const char *stage, const char *datetimeValue = nullptr);
void setupDeepSleep();
void print_wakeup_reason();
void appendFixSample(const PvtslnData &fix);
bool isRtkFixedType(const String &type);
bool isRtkFloatType(const String &type);
bool publishCycleAndStatus(const char *status);
void gnssTask(void *param);
void drainPvtQueueToBatch();
void powerOffGnssAfterAcquisition();

// =============================================================================
// SETUP
// =============================================================================
void setup()
{
  bootStarted_ms = millis();
  cycleId++;
  Serial.begin(115200);
  LOGLN(1, "********************************");
  LOGLN(1, "***** PHYSALIA2 UM980 SETUP ****");
  LOGLN(1, "********************************");
  LOGF(1, "Cycle id=%lu\n", (unsigned long)cycleId);

  setupTaskWatchdog();

  // Protection boucle de restart rapide
  {
    const uint32_t nowMs = (uint32_t)millis();
    if (lastBootWasRestart && lastRestartTimestampMs > 0) {
      const uint32_t elapsed = nowMs - lastRestartTimestampMs;
      if (elapsed < 30000UL) {
        LOGF(1, "Rapid restart loop detected (%lu ms) – forcing deep sleep\n",
             (unsigned long)elapsed);
        lastBootWasRestart     = false;
        consecutiveFailureCount = 0;
        esp_sleep_enable_timer_wakeup((uint64_t)TIME_TO_SLEEP * uS_TO_S_FACTOR);
        esp_deep_sleep_start();
      }
    }
    lastBootWasRestart = false;
  }

  // Alimentation GNSS immédiate : le temps UM980 se superpose au délai LTE.
  pinMode(PIN_GNSS_EN, OUTPUT);
  digitalWrite(PIN_GNSS_EN, HIGH);
  delay(200);

  // Démarrage LTE le plus tôt possible, sans modifier le reste du cycle validé.
  // La séquence SIM7600 démarre pendant que le setup prépare RTOS/GNSS/batterie.
  periodicModemHardReset();
  modemPowerStartEarly();

  disableUnusedBluetooth();

  pvtQueue = xQueueCreate(PVT_QUEUE_DEPTH, sizeof(PvtslnData));
  ggaMutex = xSemaphoreCreateMutex();
  if (!pvtQueue || !ggaMutex) {
    Serial.println("FATAL: RTOS alloc failed – restarting");
    feedTaskWatchdog();
    lastBootWasRestart     = true;
    lastRestartTimestampMs = (uint32_t)millis();
    ESP.restart();
  }

  print_wakeup_reason();
  setupDeepSleep();
  setupBatteryCalibration();
  recordBatterySample("boot", nullptr);

  // UM980 d'abord, puis tâche Core 0 avant la 4G : acquisition GGA/PVTSLNA en parallèle du LTE.
  setupGnss();
  xTaskCreatePinnedToCore(gnssTask, "gnssTask", 4096, nullptr, 2, nullptr, 0);
  LOGLN(1, "GNSS task started on Core 0");

  setupGsm();
  recordBatterySample("lte", nullptr);

  // MQTT est configuré ici mais connecté uniquement après l'acquisition.
  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setBufferSize(MQTT_BUFFER_SIZE);
  mqtt.setCallback(callback);

  while (Serial.available()) Serial.read();
}

// =============================================================================
// LOOP  (Core 1 – réseau + automate de cycle)
// =============================================================================
void loop()
{
  feedTaskWatchdog();
  unsigned long now = millis();

  // Vide la queue PVT produite par gnssTask (Core 0) dans le batch local.
  drainPvtQueueToBatch();

  maintainNetwork();
  maintainNtrip();
  processNtripStream();
  sendPeriodicGGA();

  now = millis();

  // MQTT n'est utilisé qu'après acquisition ; si déjà connecté, maintenir la session.
  if (mqtt.connected()) mqtt.loop();

  // Sécurité : si l'UM980 ne produit pas de GGA exploitable assez vite.
  if (DEEP_SLEEP_ACTIVATED && firstGga_ms == 0 &&
      now - bootStarted_ms > NO_GGA_TIMEOUT_MS) {
    LOGLN(1, "No GGA within energy budget – publish diagnostic and sleep");
    publishPreSleepVbat();
    publishCycleAndStatus("no_gga");
    safeDeepSleep();
  }

  // Si le caster est connecté mais aucun RTCM n'arrive rapidement.
  if (DEEP_SLEEP_ACTIVATED && ntripConnected_ms != 0 && firstRtcm_ms == 0 &&
      now - ntripConnected_ms > NO_RTCM_TIMEOUT_MS) {
    LOGLN(1, "No RTCM after NTRIP connect – publish diagnostic and sleep");
    publishPreSleepVbat();
    publishCycleAndStatus("no_rtcm");
    safeDeepSleep();
  }

  // Si les corrections arrivent mais qu'aucun RTK FIX n'est obtenu dans le budget.
  if (DEEP_SLEEP_ACTIVATED && ntripConnected_ms != 0 && rtkFixStart_ms == 0 &&
      now - ntripConnected_ms > (unsigned long)RTK_MAX_RESEARCH * 1000UL) {
    LOGLN(1, "RTK FIX not reached within search budget – publish all positions and sleep");
    publishPreSleepVbat();
    publishCycleAndStatus("no_fix");
    safeDeepSleep();
  }

  // Fin nominale : 5 s d acquisition utile après premier RTK FIX.
  if (DEEP_SLEEP_ACTIVATED && rtkFixStart_ms != 0 &&
      now - rtkFixStart_ms > (unsigned long)RTK_ACQUISITION_PERIOD * 1000UL) {
    LOGLN(1, "RTK FIX record period done - GNSS off, publish batch and deep sleep");

    // Dernière vidange de la queue avant coupure UM980 : les positions sont déjà en RAM.
    drainPvtQueueToBatch();
    feedTaskWatchdog();
    delay(25);
    drainPvtQueueToBatch();

    // À partir d ici, l acquisition est terminée : on économise l UM980 pendant MQTT.
    powerOffGnssAfterAcquisition();

    failureSleepCount       = 0;
    consecutiveFailureCount = 0;
    publishPreSleepVbat();
    publishCycleAndStatus("ok_fix");
    safeDeepSleep();
  }

  feedTaskWatchdog();
  delay(5);
}

// =============================================================================
// =============================================================================
// ACQUISITION HELPERS
// =============================================================================
void drainPvtQueueToBatch()
{
  if (!pvtQueue) return;

  PvtslnData fix;
  while (xQueueReceive(pvtQueue, &fix, 0) == pdTRUE) {
    appendFixSample(fix);
    publishBatteryIfDue(fix.datetime);  // échantillonne localement, ne publie pas encore
  }
}

void powerOffGnssAfterAcquisition()
{
  static bool gnssOffAfterAcq = false;
  if (gnssOffAfterAcq) return;
  gnssOffAfterAcq = true;

  // Le flux RTCM n est plus utile après la fenêtre de mesure.
  if (ntripClient.connected()) {
    ntripClient.stop();
    LOGLN(1, "NTRIP stopped after acquisition");
  }

  GNSSSerial.flush();
  pinMode(PIN_GNSS_EN, OUTPUT);
  digitalWrite(PIN_GNSS_EN, LOW);
  LOGF(1, "UM980 powered off after acquisition at %lu ms\n",
       (unsigned long)(millis() - bootStarted_ms));
}

// GNSS TASK  (Core 0 – lecture UART uniquement, jamais de réseau)
// =============================================================================
void gnssTask(void *param)
{
  (void)param;
  registerCurrentTaskToWatchdog("gnssTask");
  for (;;) {
    processGnssSerial();
    feedTaskWatchdog();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}