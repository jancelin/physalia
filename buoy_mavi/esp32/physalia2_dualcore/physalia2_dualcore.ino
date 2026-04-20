/*
=============================================================================================
PHYSALIA - UM980 edition
  - LTE NTRIP rover over SIM7600
  - GNSS telemetry over MQTT

Adaptation notes:
  - u-blox / SparkFun code removed completely
  - UM980 uses UART on Serial2 at 460800 baud
  - RTCM corrections are forwarded directly to UM980 over UART
  - GGA is read from UM980 and forwarded to the caster
  - PVTSLNA is parsed and published to MQTT

Architecture (v2 - dual-core):
  - Core 0  : gnssTask  – reads GNSSSerial continuously, never blocked by network
  - Core 1  : loop()    – all network (NTRIP + MQTT), drains PVT queue
  - pvtQueue : FreeRTOS queue, PvtslnData producer→consumer between cores
  - ggaMutex : protects lastGGASentence accessed from both cores
=============================================================================================
*/

#include <Arduino.h>

#include "secrets.h"
#include "NTRIPClient.h"

#include <SPI.h>
#include <esp_adc_cal.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <time.h>
#include <esp_task_wdt.h>

// FreeRTOS (already included by ESP32 Arduino core)
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Pour desactiver BT
#if defined(ARDUINO_ARCH_ESP32) && __has_include("esp32-hal-bt.h")
  #include "esp32-hal-bt.h"
#endif
// -------------------------------------------------------------------------------------------------
// TinyGSM compile-time configuration
// Must be defined BEFORE including <TinyGsmClient.h>
// -------------------------------------------------------------------------------------------------

#define TINY_GSM_RX_BUFFER 1024
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_DEBUG Serial
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

#include <TinyGsmClient.h>

// -------------------------------------------------------------------------------------------------
// Log macros  (LOG_LEVEL défini dans secrets.h : 0=rien, 1=minimal, 2=complet)
// -------------------------------------------------------------------------------------------------
// LOGLN / LOG  : println / print d'un message fixe
// LOGF         : printf formaté (ex: LOGF(1, "val=%d\n", x))
// LOGBLOCK(lvl){ ... } : bloc multi-lignes conditionnel
// -------------------------------------------------------------------------------------------------
#define LOGLN(lvl, msg)        do { if (LOG_LEVEL >= (lvl)) Serial.println(msg); } while(0)
#define LOG(lvl, msg)          do { if (LOG_LEVEL >= (lvl)) Serial.print(msg);   } while(0)
#define LOGF(lvl, fmt, ...)    do { if (LOG_LEVEL >= (lvl)) Serial.printf(fmt, ##__VA_ARGS__); } while(0)
#define LOGBLOCK(lvl)          if (LOG_LEVEL >= (lvl))

// -------------------------------------------------------------------------------------------------
// Hardware mapping
// -------------------------------------------------------------------------------------------------

// GNSS power enable (Pololu 2810 ON pin)
#define PIN_GNSS_EN 23

// GNSS UART (ESP32 <-> UM980)
#define UM980_TX_PIN 22 // ESP32 TX -> UM980 RXD2
#define UM980_RX_PIN 21 // ESP32 RX <- UM980 TXD2
#define UM980_BAUD   460800

// SIM7600 UART and power
#define UART_BAUD 115200
#define PIN_TX    27
#define PIN_RX    26
#define PWR_PIN   4
#define LED_PIN   12
#define POWER_PIN 25
#define IND_PIN   36

// Battery / ADC
// LilyGO T-SIM7600G-H R2 : Li-Ion 18650 → diviseur 100kΩ/100kΩ → GPIO35 (ADC1_CH7, input-only).
// Le rapport du diviseur est configurable via VBAT_ADC_DIVIDER_RATIO dans secrets.h.
// GPIO4 = PWR_PIN (sortie numérique modem) → ne peut PAS être ADC.
// AT+CBC lit la tension régulée du modem (~4.1V fixe), inutile pour la décharge Li-Ion.
#define ADC_PIN     35

// -------------------------------------------------------------------------------------------------
// TinyGSM
// -------------------------------------------------------------------------------------------------

#define SerialAT Serial1

#define GSM_PIN ""
const char apn[]      = "sl2sfr";
const char gprsUser[] = "";
const char gprsPass[] = "";

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient mqttTransport(modem, 0);
TinyGsmClient ntripTransport(modem, 2);
PubSubClient mqtt(mqttTransport);
NTRIPClient ntripClient(ntripTransport);
HardwareSerial GNSSSerial(2);

// -------------------------------------------------------------------------------------------------
// Runtime state
// -------------------------------------------------------------------------------------------------

static const uint32_t TASK_WDT_TIMEOUT_S = 60;
static const uint8_t FAILURE_REBOOT_THRESHOLD = 3;
static bool taskWdtEnabled = false;

RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR uint8_t consecutiveFailureCount = 0;

// -------------------------------------------------------------------------------------------------
// Battery monitoring – variables persistant en RTC RAM à travers les deep-sleeps
// -------------------------------------------------------------------------------------------------
// rtcVbatMinMv   : pire tension brute observée depuis le dernier boot froid.
//                  Révèle la tension sous charge maximale, indicateur de fin de vie.
// rtcVbatEmaMv   : EMA asymétrique (α_down=1/8, α_up=1/4, τ≈80/40 min à 10 min sleep).
//                  Tendance lente de décharge et détection rapide de recharge solaire.
// rtcVbatPrevEmaMv : EMA du cycle précédent → permet de calculer vbat_delta_mv.
//                    Positif = recharge, Négatif = décharge.
// rtcVbatEmaInit : false uniquement au boot froid ; évite d'initialiser l'EMA avec 0.
RTC_DATA_ATTR int32_t  rtcVbatMinMv     = 9999;
RTC_DATA_ATTR int32_t  rtcVbatEmaMv     = 0;
RTC_DATA_ATTR int32_t  rtcVbatPrevEmaMv = 0;
RTC_DATA_ATTR bool     rtcVbatEmaInit   = false;

// cycleVbatMinMv : minimum de tension observé pendant le cycle en cours.
// Réinitialisé à chaque réveil (variable non-RTC, pas besoin de persistance).
// Représente la pire tension sous charge du cycle courant → courbe de décharge réelle.
static int32_t cycleVbatMinMv = 9999;

// Caractéristiques de calibration ADC (calculées une fois dans setupBatteryCalibration,
// utilisées dans readVbatMv() à chaque réveil).
static esp_adc_cal_characteristics_t adcChars;
static bool adcCalibrated = false;

int vref = 1100;
uint32_t timeStamp = 0;
unsigned long lastReconnectAttempt = 0;
unsigned long lastReceivedRTCM_ms = 0;
unsigned long lastGgaToCaster_ms = 0;
unsigned long lastPvtsln_ms = 0;
unsigned long lastState = 0;

const unsigned long maxTimeBeforeHangup_ms = 10000UL;
const size_t GNSS_LINE_BUFFER_SIZE = 1024;
char gnssLine[GNSS_LINE_BUFFER_SIZE];
size_t gnssLineLen = 0;
String lastGGASentence;        // protected by ggaMutex

struct PvtslnData {
  bool valid = false;
  char datetime[32] = {0};
  String bestposType;
  double lat = 0.0;
  double lon = 0.0;
  double altMSL = 0.0;
  double undulation = 0.0;
  double ellipsoid = 0.0;
  double velNorth = 0.0;
  double velEast = 0.0;
  double velGround = 0.0;
  float hgtStd = 0.0f;
  float latStd = 0.0f;
  float lonStd = 0.0f;
  float diffAge = 0.0f;
  String headingType;
  float headingLength = 0.0f;
  float headingDeg = 0.0f;
  float headingPitch = 0.0f;
  float gdop = 0.0f;
  float pdop = 0.0f;
  float hdop = 0.0f;
  float htdop = 0.0f;
  float tdop = 0.0f;
  uint8_t bestposTrackedSvs = 0;
  uint8_t bestposSolnSvs = 0;
  uint8_t psrposTrackedSvs = 0;
  uint8_t psrposSolnSvs = 0;
  uint8_t headingTrackedSvs = 0;
  uint8_t headingSolnSvs = 0;
  uint8_t headingGgl1 = 0;
  uint8_t headingGgl1l2 = 0;
};

PvtslnData lastFix;

// -------------------------------------------------------------------------------------------------
// FreeRTOS inter-task communication
// -------------------------------------------------------------------------------------------------

// Depth = 20 frames → 4 s buffer at 5 Hz (absorbs MQTT bursts)
static const uint8_t PVT_QUEUE_DEPTH = 20;
static QueueHandle_t   pvtQueue  = nullptr;  // PvtslnData: Core 0 → Core 1
static SemaphoreHandle_t ggaMutex = nullptr; // guards lastGGASentence

// -------------------------------------------------------------------------------------------------
// Forward declarations
// -------------------------------------------------------------------------------------------------

void callback(char* topic, byte* payload, unsigned int length);
boolean reconnect();
void print_wakeup_reason();
void modem_on();
void modem_off();
void setupGsm();
void setupBatteryCalibration();
void setupDeepSleep();
void setupGnss();
void setupTaskWatchdog();
void registerCurrentTaskToWatchdog(const char *taskName);
void feedTaskWatchdog();
void resetFailureCycleCounter(const char *reason);
void handleFailureCycleAndSleep(const char *stage);
void maintainNetwork();
void maintainNtrip();
void processGnssSerial();
void processNtripStream();
void processGnssLine(const char *line);
void sendPeriodicGGA();
void publishFix(const PvtslnData &fix);
void publishBatteryIfDue(const char *datetimeValue);
int32_t readVbatMv();
void publishPreSleepVbat();
void publishNtripConnectVbat();
bool parsePvtslnLine(const char *line, PvtslnData &out);
bool gpsWeekTowToUtcString(uint16_t gpsWeek, uint32_t towMs, int leapSeconds, char *out, size_t outSize);
int splitCsv(char *str, char **tokens, int maxTokens);
int mapPositionTypeToFix(const String &positionType);
int mapPositionTypeToCarrier(const String &positionType);
void gnssTask(void *param);

// Pour desactiver BT
void disableUnusedBluetooth()
{
#if defined(ARDUINO_ARCH_ESP32) && __has_include("esp32-hal-bt.h")
  // Firmware Physalia : Bluetooth non utilisé.
  // On coupe le contrôleur BT très tôt pour réduire la conso réveillée
  // et éviter de garder de la RAM occupée inutilement.
  if (btStarted()) {
    if (btStop()) {
      LOGLN(1, "Bluetooth controller stopped");
    } else {
      LOGLN(1, "Bluetooth controller stop failed");
    }
  } else {
    LOGLN(1, "Bluetooth already stopped");
  }
#endif
}

// -------------------------------------------------------------------------------------------------
// Task watchdog (enabled only in deep-sleep mode)
// -------------------------------------------------------------------------------------------------

void setupTaskWatchdog()
{
  if (!DEEP_SLEEP_ACTIVATED) {
    return;
  }

  esp_err_t err = ESP_OK;

#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)
  esp_task_wdt_config_t twdtConfig = {};
  twdtConfig.timeout_ms = TASK_WDT_TIMEOUT_S * 1000UL;
  twdtConfig.idle_core_mask = 0;
  twdtConfig.trigger_panic = true;
  err = esp_task_wdt_init(&twdtConfig);
  if (err == ESP_ERR_INVALID_STATE) {
    err = esp_task_wdt_reconfigure(&twdtConfig);
  }
#else
  err = esp_task_wdt_init(TASK_WDT_TIMEOUT_S, true);
#endif

  if (err != ESP_OK) {
    LOGF(1, "TWDT init failed: %d\n", static_cast<int>(err));
    return;
  }

  taskWdtEnabled = true;
  registerCurrentTaskToWatchdog("loopTask");
  LOGLN(1, "TWDT enabled (60 s)");
}

void registerCurrentTaskToWatchdog(const char *taskName)
{
  if (!taskWdtEnabled) {
    return;
  }

  const esp_err_t err = esp_task_wdt_add(nullptr);
  if (err == ESP_OK) {
    LOGF(1, "TWDT subscribed: %s\n", taskName ? taskName : "current");
  } else if (err != ESP_ERR_INVALID_ARG) {
    LOGF(1, "TWDT subscribe failed for %s: %d\n", taskName ? taskName : "current", static_cast<int>(err));
  }
}

void feedTaskWatchdog()
{
  if (!taskWdtEnabled) {
    return;
  }
  (void)esp_task_wdt_reset();
}

void resetFailureCycleCounter(const char *reason)
{
  if (consecutiveFailureCount == 0) {
    return;
  }

  LOGF(1, "Failure cycle counter reset (%s): %u -> 0\n",
       reason ? reason : "success",
       static_cast<unsigned>(consecutiveFailureCount));
  consecutiveFailureCount = 0;
}

void handleFailureCycleAndSleep(const char *stage)
{
  ++consecutiveFailureCount;
  LOGF(1,
       "Failure cycle recorded at stage %s: %u/%u\n",
       stage ? stage : "unknown",
       static_cast<unsigned>(consecutiveFailureCount),
       static_cast<unsigned>(FAILURE_REBOOT_THRESHOLD));

  if (consecutiveFailureCount >= FAILURE_REBOOT_THRESHOLD) {
    LOGLN(1, "Failure threshold reached, forcing full ESP restart");
    delay(100);
    ESP.restart();
  }

  if (DEEP_SLEEP_ACTIVATED) {
    modem_off();
    delay(2000);
    esp_deep_sleep_start();
  }
}

// -------------------------------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  LOGLN(1, "********************************");
  LOGLN(1, "***** PHYSALIA UM980 SETUP *****");
  LOGLN(1, "********************************");

  setupTaskWatchdog();

  // GNSS power rail via Pololu 2810
  pinMode(PIN_GNSS_EN, OUTPUT);
  digitalWrite(PIN_GNSS_EN, HIGH);
  delay(200);
  
  // Pour desactiver BT
  disableUnusedBluetooth();

  // ── Create FreeRTOS primitives BEFORE setupGnss() ─────────────────────────
  pvtQueue  = xQueueCreate(PVT_QUEUE_DEPTH, sizeof(PvtslnData));
  ggaMutex  = xSemaphoreCreateMutex();
  if (!pvtQueue || !ggaMutex) {
    Serial.println("FATAL: failed to allocate RTOS primitives – restarting");
    feedTaskWatchdog();
    ESP.restart();
  }
  // ──────────────────────────────────────────────────────────────────────────

  print_wakeup_reason();
  setupDeepSleep();
  setupGsm();
  setupBatteryCalibration();
  setupGnss();   // calls processGnssSerial() internally (single-core, safe)

  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setBufferSize(1024);
  mqtt.setCallback(callback);

  unsigned long start = millis();
  while (!mqtt.connected() && (millis() - start < ACQUISION_PERIOD_MQTT)) {
    feedTaskWatchdog();
    LOGLN(1, "Connecting to MQTT...");
    if (mqtt.connect(matUuid, mqttUser, mqttPassword)) {
      LOGLN(1, "MQTT connected");
      break;
    }
    LOGF(1, "MQTT failed with state %d\n", mqtt.state());
    feedTaskWatchdog();
    delay(1500);
  }

  if (mqtt.connected()) {
    resetFailureCycleCounter("startup complete");
  }

  if (DEEP_SLEEP_ACTIVATED && !mqtt.connected()) {
    LOGLN(1, "Max period attempted to connect to MQTT, DeepSleep activated");
    handleFailureCycleAndSleep("MQTT");
  }

  while (Serial.available()) {
    Serial.read();
  }

  // ── Spawn GNSS task on Core 0 (network/MQTT stays on Core 1 / loop()) ─────
  // Priority 2 > loop priority 1 → GNSS reads fire immediately when bytes arrive
  xTaskCreatePinnedToCore(
      gnssTask,   // function
      "gnssTask", // name
      4096,       // stack in words
      nullptr,    // parameter
      2,          // priority
      nullptr,    // handle (not needed)
      0           // Core 0
  );
  LOGLN(1, "GNSS task started on Core 0");
  // ──────────────────────────────────────────────────────────────────────────
}

// -------------------------------------------------------------------------------------------------
// Loop  (Core 1 – all network operations)
// -------------------------------------------------------------------------------------------------

void loop()
{
  feedTaskWatchdog();
  unsigned long now = millis();

  // Watchdog uniquement en mode deep-sleep : en continu, pas de période d'acquisition à respecter
  if (DEEP_SLEEP_ACTIVATED && lastState != 0 && now - lastState > static_cast<unsigned long>(RTK_ACQUISITION_PERIOD * 1000UL * 1.2f)) {
    LOGLN(1, "Safety watchdog: acquisition period exceeded, restarting");
    ESP.restart();
  }

  // ── Drain the PVT queue produced by gnssTask on Core 0 ───────────────────
  // Each dequeued frame is published immediately; if MQTT is slow the queue
  // absorbs the backlog (up to PVT_QUEUE_DEPTH frames ≈ 4 s at 5 Hz).
  {
    PvtslnData fix;
    while (xQueueReceive(pvtQueue, &fix, 0) == pdTRUE) {
      publishFix(fix);
      publishBatteryIfDue(fix.datetime);
    }
  }
  // ──────────────────────────────────────────────────────────────────────────

  maintainNetwork();
  maintainNtrip();
  processNtripStream();
  sendPeriodicGGA();
  
  // IMPORTANT :
  // maintainNtrip() peut avoir mis lastState = millis() dans cette même itération.
  // Il faut donc recalculer "now" après maintainNtrip() pour éviter un wrap-around.
  now = millis();

  if (!mqtt.connected()) {
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    mqtt.loop();
  }



  if (DEEP_SLEEP_ACTIVATED && lastState != 0 && now - lastState > static_cast<unsigned long>(RTK_ACQUISITION_PERIOD * 1000UL)) {
    LOGLN(1, "Record period done, entering deep sleep");
    publishPreSleepVbat(); // mesure sous charge max avant extinction modem
    modem_off();
    delay(2000);
    esp_deep_sleep_start();
  }

  feedTaskWatchdog();
  delay(5);
}

// -------------------------------------------------------------------------------------------------
// GNSS task  (Core 0 – reads UART, never touches the modem)
// -------------------------------------------------------------------------------------------------

void gnssTask(void *param)
{
  (void)param;
  registerCurrentTaskToWatchdog("gnssTask");
  for (;;) {
    processGnssSerial();          // drains GNSSSerial RX buffer
    feedTaskWatchdog();
    vTaskDelay(pdMS_TO_TICKS(1)); // 1 ms yield – keeps Core 0 responsive
  }
}

// -------------------------------------------------------------------------------------------------
// MQTT
// -------------------------------------------------------------------------------------------------

void callback(char* topic, byte* payload, unsigned int length)
{
  LOGBLOCK(2) {
    Serial.print("Message arrived on topic: ");
    Serial.println(topic);
    for (unsigned int i = 0; i < length; i++) {
      Serial.print(static_cast<char>(payload[i]));
    }
    Serial.println();
  }
}

boolean reconnect()
{
  feedTaskWatchdog();
  if (mqtt.connect(matUuid, mqttUser, mqttPassword)) {
    LOGLN(1, "MQTT reconnected");
    return true;
  }

  LOGF(1, "MQTT reconnect failed, state=%d\n", mqtt.state());
  return false;
}

// -------------------------------------------------------------------------------------------------
// GSM / modem
// -------------------------------------------------------------------------------------------------

void setupGsm()
{
  delay(10);
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);

  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  delay(500);
  digitalWrite(PWR_PIN, LOW);

  LOGLN(1, "Initializing modem...");
  feedTaskWatchdog();
  if (!modem.testAT()) {
    LOGLN(1, "Failed to test modem, attempting to continue without restarting");
  }

  LOG(1, "Waiting for network...");
  unsigned long start = millis();
  while (!modem.waitForNetwork() && (millis() - start < ACQUISION_PERIOD_4G)) {
    feedTaskWatchdog();
    LOGLN(1, "fail to find network, waiting 10 sec before retry");
    delay(10000);
    feedTaskWatchdog();
  }

  if (!modem.isNetworkConnected()) {
    LOGLN(1, "Network connection failed");
    if (DEEP_SLEEP_ACTIVATED) {
      handleFailureCycleAndSleep("4G");
    }
  } else {
    LOGLN(1, "Network connected");
  }

#if TINY_GSM_USE_GPRS
  LOG(1, F("Connecting to APN: "));
  LOG(1, apn);
  feedTaskWatchdog();
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    LOGLN(1, " fail");
    if (DEEP_SLEEP_ACTIVATED) {
      handleFailureCycleAndSleep("GPRS");
    }
  }
  LOGLN(1, " success");

  if (modem.isGprsConnected()) {
    LOGLN(1, "GPRS connected");
  }
#endif
}

void maintainNetwork()
{
  unsigned long now = millis();

  if (!modem.isNetworkConnected()) {
    unsigned long start = millis();
    LOGLN(1, "LOOP - Network disconnected");

    while (!modem.waitForNetwork() && (millis() - start < ACQUISION_PERIOD_4G)) {
      feedTaskWatchdog();
      LOGLN(1, "LOOP - fail to find network, waiting 10 sec before retry");
      delay(10000);
      feedTaskWatchdog();
    }

    if (!modem.isNetworkConnected()) {
      if (DEEP_SLEEP_ACTIVATED) {
        LOGLN(1, "LOOP - Max period attempted to connect to 4G, DeepSleep activated");
        handleFailureCycleAndSleep("4G");
      }
    } else {
      LOGLN(1, "LOOP - Network re-connected");
    }
  }

#if TINY_GSM_USE_GPRS
  if (!modem.isGprsConnected()) {
    LOGLN(1, "GPRS disconnected!");
    LOG(1, F("Connecting to "));
    LOGLN(1, apn);
    feedTaskWatchdog();
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      LOGLN(1, "GPRS reconnect failed");
      delay(10000);
      return;
    }
    if (modem.isGprsConnected()) {
      LOGLN(1, "GPRS reconnected");
    }
  }
#endif

  (void)now;
}

void modem_on()
{
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  delay(10);
  digitalWrite(POWER_PIN, LOW);
  delay(1010);
  digitalWrite(POWER_PIN, HIGH);
  LOGLN(1, "Waiting till modem ready...");
  feedTaskWatchdog();
  delay(4510);
  feedTaskWatchdog();
}

void modem_off()
{
  LOGLN(1, "Going to sleep now with modem turned off");
  modem.sleepEnable(false);
  modem.poweroff();
}

// -------------------------------------------------------------------------------------------------
// GNSS / UM980
// -------------------------------------------------------------------------------------------------

void setupGnss()
{
  // Enlarge RX buffer BEFORE begin() – at 460800 baud the default 256 B fills
  // in ~4 ms; 4096 B gives ~70 ms of headroom during modem operations.
  GNSSSerial.setRxBufferSize(4096);
  GNSSSerial.begin(UM980_BAUD, SERIAL_8N1, UM980_RX_PIN, UM980_TX_PIN);
  LOGLN(1, "UM980 UART started on Serial2 @ 460800");

  unsigned long start = millis();
  bool receivedSomething = false;
  while (millis() - start < ACQUISION_PERIOD_GNSS) {
    feedTaskWatchdog();
    processGnssSerial();
    if (lastPvtsln_ms != 0 || lastGGASentence.length() > 0) {
      receivedSomething = true;
      break;
    }
    delay(20);
  }

  if (!receivedSomething) {
    LOGLN(1, "No UM980 output detected within GNSS acquisition window");
    if (DEEP_SLEEP_ACTIVATED) {
      handleFailureCycleAndSleep("GNSS");
    }
  } else {
    LOGLN(1, "UM980 output detected");
  }
}

// Called from gnssTask (Core 0) during normal operation,
// and from the main task during setupGnss() before gnssTask is created.
void processGnssSerial()
{
  while (GNSSSerial.available()) {
    char c = static_cast<char>(GNSSSerial.read());

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      if (gnssLineLen > 0) {
        gnssLine[gnssLineLen] = '\0';
        processGnssLine(gnssLine);
        gnssLineLen = 0;
      }
      continue;
    }

    if (gnssLineLen < GNSS_LINE_BUFFER_SIZE - 1) {
      gnssLine[gnssLineLen++] = c;
    } else {
      gnssLineLen = 0;
    }
  }
}

// ── MODIFIED: GGA protected by mutex; PVTSLNA enqueued instead of published ─
void processGnssLine(const char *line)
{
  if (line == nullptr || line[0] == '\0') {
    return;
  }

  // --- GGA sentence ---
  if ((strncmp(line, "$GPGGA", 6) == 0) || (strncmp(line, "$GNGGA", 6) == 0)) {
    String gga = String(line) + "\r\n";
    // ggaMutex may be nullptr during very early boot (before setup creates it)
    if (ggaMutex && xSemaphoreTake(ggaMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      lastGGASentence = gga;
      ntripClient.setLastGGA(lastGGASentence);
      xSemaphoreGive(ggaMutex);
    } else if (!ggaMutex) {
      // Pre-task init path (setupGnss called before pvtQueue created) – no lock needed
      lastGGASentence = gga;
      ntripClient.setLastGGA(lastGGASentence);
    }
    return;
  }

  // --- PVTSLNA sentence ---
  if (strncmp(line, "#PVTSLNA", 8) == 0) {
    PvtslnData parsed;
    if (parsePvtslnLine(line, parsed)) {
      lastFix = parsed;
      lastPvtsln_ms = millis();

      if (pvtQueue) {
        // Non-blocking send; if queue is full, discard the oldest frame
        if (xQueueSend(pvtQueue, &parsed, 0) != pdTRUE) {
          PvtslnData discarded;
          xQueueReceive(pvtQueue, &discarded, 0);
          xQueueSend(pvtQueue, &parsed, 0);
          LOGLN(1, "[GNSS] PVT queue full – oldest frame dropped");
        }
      } else {
        // Pre-task init path: publish directly (single-core, safe)
        publishFix(parsed);
        publishBatteryIfDue(parsed.datetime);
      }
    }
  }
}
// ────────────────────────────────────────────────────────────────────────────

bool parsePvtslnLine(const char *line, PvtslnData &out)
{
  if (line == nullptr) {
    return false;
  }

  char buffer[GNSS_LINE_BUFFER_SIZE];
  strncpy(buffer, line, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';

  char *crc = strchr(buffer, '*');
  if (crc != nullptr) {
    *crc = '\0';
  }

  char *semicolon = strchr(buffer, ';');
  if (semicolon == nullptr) {
    return false;
  }

  *semicolon = '\0';
  char *header = buffer;
  char *payload = semicolon + 1;

  char *headerTokens[16] = {0};
  char *payloadTokens[96] = {0};

  int headerCount = splitCsv(header, headerTokens, 16);
  int payloadCount = splitCsv(payload, payloadTokens, 96);

  if (headerCount < 9 || payloadCount < 33) {
    return false;
  }

  const uint16_t gpsWeek = static_cast<uint16_t>(atoi(headerTokens[4]));
  const uint32_t towMs = static_cast<uint32_t>(strtoul(headerTokens[5], nullptr, 10));
  const int leapSeconds = atoi(headerTokens[8]);
  if (!gpsWeekTowToUtcString(gpsWeek, towMs, leapSeconds, out.datetime, sizeof(out.datetime))) {
    strncpy(out.datetime, "1970-01-01 00:00:00.000", sizeof(out.datetime) - 1);
  }

  out.bestposType = payloadTokens[0];
  out.altMSL = strtod(payloadTokens[1], nullptr);
  out.lat = strtod(payloadTokens[2], nullptr);
  out.lon = strtod(payloadTokens[3], nullptr);
  out.hgtStd = strtof(payloadTokens[4], nullptr);
  out.latStd = strtof(payloadTokens[5], nullptr);
  out.lonStd = strtof(payloadTokens[6], nullptr);
  out.diffAge = strtof(payloadTokens[7], nullptr);
  out.undulation = strtod(payloadTokens[12], nullptr);
  out.ellipsoid = out.altMSL + out.undulation;
  out.bestposTrackedSvs = static_cast<uint8_t>(atoi(payloadTokens[13]));
  out.bestposSolnSvs = static_cast<uint8_t>(atoi(payloadTokens[14]));
  out.psrposTrackedSvs = static_cast<uint8_t>(atoi(payloadTokens[15]));
  out.psrposSolnSvs = static_cast<uint8_t>(atoi(payloadTokens[16]));
  out.velNorth = strtod(payloadTokens[17], nullptr);
  out.velEast = strtod(payloadTokens[18], nullptr);
  out.velGround = strtod(payloadTokens[19], nullptr);
  out.headingType = payloadTokens[20];
  out.headingLength = strtof(payloadTokens[21], nullptr);
  out.headingDeg = strtof(payloadTokens[22], nullptr);
  out.headingPitch = strtof(payloadTokens[23], nullptr);
  out.headingTrackedSvs = static_cast<uint8_t>(atoi(payloadTokens[24]));
  out.headingSolnSvs = static_cast<uint8_t>(atoi(payloadTokens[25]));
  out.headingGgl1 = static_cast<uint8_t>(atoi(payloadTokens[26]));
  out.headingGgl1l2 = static_cast<uint8_t>(atoi(payloadTokens[27]));
  out.gdop = strtof(payloadTokens[28], nullptr);
  out.pdop = strtof(payloadTokens[29], nullptr);
  out.hdop = strtof(payloadTokens[30], nullptr);
  out.htdop = strtof(payloadTokens[31], nullptr);
  out.tdop = strtof(payloadTokens[32], nullptr);
  out.valid = true;

  return true;
}

void publishFix(const PvtslnData &fix)
{
  if (!mqtt.connected() || !fix.valid) {
    return;
  }

  StaticJsonDocument<768> doc;
  doc["capteur"] = matUuid;

  String datetimeSql = String("'") + fix.datetime + String("'");
  doc["datetime"] = datetimeSql;
  doc["lat"] = fix.lat;
  doc["lon"] = fix.lon;
  doc["elv_m"] = fix.ellipsoid;
  doc["alt_m"] = fix.altMSL;
  doc["undulation_m"] = fix.undulation;
  doc["fix"] = mapPositionTypeToFix(fix.bestposType);
  doc["fix_type"] = fix.bestposType;
  doc["car"] = mapPositionTypeToCarrier(fix.bestposType);
  doc["hacc_mm"] = static_cast<uint32_t>(max(fix.latStd, fix.lonStd) * 1000.0f);
  doc["vacc_mm"] = static_cast<uint32_t>(fix.hgtStd * 1000.0f);
  doc["latstd_m"] = fix.latStd;
  doc["lonstd_m"] = fix.lonStd;
  doc["hgtstd_m"] = fix.hgtStd;
  doc["numsv"] = fix.bestposSolnSvs;
  doc["sv_tracked"] = fix.bestposTrackedSvs;
  doc["diffage_s"] = fix.diffAge;
  doc["vel_n_ms"] = fix.velNorth;
  doc["vel_e_ms"] = fix.velEast;
  doc["spd_ms"] = fix.velGround;
  doc["heading_type"] = fix.headingType;
  doc["heading_deg"] = fix.headingDeg;
  doc["pitch_deg"] = fix.headingPitch;
  doc["gdop"] = fix.gdop;
  doc["pdop"] = fix.pdop;
  doc["hdop"] = fix.hdop;

  String msg;
  serializeJson(doc, msg);

  LOGF(2, "Geo payload bytes: %u\n", msg.length());

  if (!mqtt.publish(mqtttopic, msg.c_str())) {
    LOGF(1, "MQTT geo publish failed, state=%d, buffer=%d\n%s\n",
         mqtt.state(), mqtt.getBufferSize(), msg.c_str());
    return;
  }

  LOGLN(1, "Geo publish OK");
  LOGLN(2, msg);
}

// -------------------------------------------------------------------------------------------------
// readVbatMv() – lecture robuste de la tension LiPo via GPIO35 (diviseur 100k/100k)
//
// Stratégie :
//   16 échantillons bruts → tri insertion → médiane des 8 centraux (rejet 4 min + 4 max)
//   → calibration esp_adc_cal → ×2 (diviseur)
//
// L'ADC ESP32 est non-linéaire aux extrêmes et bruité.
// La médiane sur 16 échantillons élimine les spikes (modem TX, WiFi, etc.)
// sans alourdir le calcul. Durée totale : ~2 ms.
// -------------------------------------------------------------------------------------------------
int32_t readVbatMv()
{
  static const uint8_t N       = 16; // nombre d'échantillons
  static const uint8_t TRIM    = 4;  // échantillons rejetés de chaque côté
  static const uint8_t KEPT    = N - 2 * TRIM; // 8 échantillons utilisés pour la médiane

  uint32_t samples[N];

  // Acquisition
  for (uint8_t i = 0; i < N; i++) {
    samples[i] = static_cast<uint32_t>(analogRead(ADC_PIN));
    delayMicroseconds(200); // laisser l'ADC se stabiliser entre les lectures
  }

  // Tri insertion (rapide pour N=16)
  for (uint8_t i = 1; i < N; i++) {
    const uint32_t key = samples[i];
    int8_t j = static_cast<int8_t>(i) - 1;
    while (j >= 0 && samples[j] > key) {
      samples[j + 1] = samples[j];
      j--;
    }
    samples[j + 1] = key;
  }

  // Médiane des KEPT valeurs centrales
  uint64_t sum = 0;
  for (uint8_t i = TRIM; i < N - TRIM; i++) {
    sum += samples[i];
  }
  const uint32_t rawMedian = static_cast<uint32_t>(sum / KEPT);

  // Conversion via calibration eFuse (ou fallback 1100 mV)
  uint32_t halfVbatMv;
  if (adcCalibrated) {
    halfVbatMv = esp_adc_cal_raw_to_voltage(rawMedian, &adcChars);
  } else {
    // Fallback linéaire brut (moins précis, utilisé si calibration non disponible)
    halfVbatMv = static_cast<uint32_t>((rawMedian * 3300UL) / 4095UL);
  }

  // Multiplication par le rapport du diviseur résistif (configurable dans secrets.h)
  return static_cast<int32_t>(halfVbatMv * static_cast<uint32_t>(VBAT_ADC_DIVIDER_RATIO));
}

// -------------------------------------------------------------------------------------------------
// publishBatteryIfDue() – publiée à intervalles réguliers pendant l'acquisition
//
// Payload MQTT (topic mqttbat) :
//   vbat_mv          : tension brute médiane courante (GPIO35, calibrée)
//   vbat_ema_mv      : EMA asymétrique (α_down=1/8, α_up=1/4)
//   vbat_delta_mv    : EMA_courante - EMA_cycle_précédent (>0=recharge, <0=décharge)
//   vbat_min_mv      : pire tension absolue depuis boot froid (historique multi-jours)
//   vbat_min_cycle_mv: pire tension du cycle en cours (courbe de décharge au cycle)
//   boot_count       : numéro de cycle depuis le boot froid (corrélation cycle/tension)
//   charge_state     : 0=décharge, 1=en charge, 2=plein (AT+CBC, détecte USB/solaire)
//   pre_sleep        : false (distingue des mesures avant extinction – topic identique)
//   alert            : true si tension < VBAT_CRITICAL_MV (3600 mV ≈ 10% Li-Ion 18650)
//   trigger          : "periodic" (origine de la mesure)
// -------------------------------------------------------------------------------------------------
void publishBatteryIfDue(const char *datetimeValue)
{
  if (!mqtt.connected()) {
    return;
  }

  if (millis() - timeStamp <= static_cast<unsigned long>(BAT_PERIOD * 1000UL)) {
    return;
  }

  timeStamp = millis();

  // Lecture tension Li-Ion via GPIO35
  const int32_t vbatMv = readVbatMv();
  if (vbatMv < 2000 || vbatMv > 5000) {
    LOGF(1, "Vbat hors plage ignorée: %d mV\n", static_cast<int>(vbatMv));
    return;
  }

  // Mise à jour minimum du cycle courant
  if (vbatMv < cycleVbatMinMv) {
    cycleVbatMinMv = vbatMv;
  }

  // Mise à jour minimum absolu (pire tension historique depuis boot froid)
  if (vbatMv < rtcVbatMinMv) {
    rtcVbatMinMv = vbatMv;
  }

  // EMA asymétrique : α_down=1/8 (τ≈80min), α_up=1/4 (τ≈40min)
  // Réaction plus rapide à la hausse → détection de recharge solaire.
  // Réaction plus lente à la baisse → lissage du bruit de décharge.
  rtcVbatPrevEmaMv = rtcVbatEmaMv; // snapshot avant mise à jour pour vbat_delta_mv
  if (!rtcVbatEmaInit) {
    rtcVbatEmaMv   = vbatMv;
    rtcVbatEmaInit = true;
  } else if (vbatMv >= rtcVbatEmaMv) {
    rtcVbatEmaMv = rtcVbatEmaMv + (vbatMv - rtcVbatEmaMv) / 4;  // hausse rapide (recharge)
  } else {
    rtcVbatEmaMv = rtcVbatEmaMv + (vbatMv - rtcVbatEmaMv) / 8;  // baisse lente (décharge)
  }

  const int32_t vbatDeltaMv = rtcVbatEmaMv - rtcVbatPrevEmaMv;

  // Charge state via AT+CBC (valide pour détecter solaire/USB branché)
  int8_t chargeState = 0;
  int8_t percent     = 0;
  int16_t modemMv    = 0;
  modem.getBattStats(chargeState, percent, modemMv);

  StaticJsonDocument<448> doc;
  doc["capteur"]           = matUuid;
  doc["datetime"]          = datetimeValue;
  doc["vbat_mv"]           = vbatMv;
  doc["vbat_ema_mv"]       = static_cast<int32_t>(rtcVbatEmaMv);
  doc["vbat_delta_mv"]     = vbatDeltaMv;
  doc["vbat_min_mv"]       = static_cast<int32_t>(rtcVbatMinMv);
  doc["vbat_min_cycle_mv"] = static_cast<int32_t>(cycleVbatMinMv);
  doc["boot_count"]        = static_cast<uint32_t>(bootCount);
  doc["charge_state"]      = chargeState;
  doc["pre_sleep"]         = false;
  doc["alert"]             = (vbatMv < VBAT_CRITICAL_MV);
  doc["trigger"]           = "periodic";

  String msg;
  serializeJson(doc, msg);

  if (!mqtt.publish(mqttbat, msg.c_str())) {
    LOGF(1, "MQTT bat publish failed, state=%d, buffer=%d\n%s\n",
         mqtt.state(), mqtt.getBufferSize(), msg.c_str());
    return;
  }

  LOGLN(1, "Battery publish OK");
  LOGF(1, "  vbat=%d mV  ema=%d mV  delta=%+d mV  min_cycle=%d mV  min=%d mV  charge=%d  alert=%d\n",
       static_cast<int>(vbatMv),
       static_cast<int>(rtcVbatEmaMv),
       static_cast<int>(vbatDeltaMv),
       static_cast<int>(cycleVbatMinMv),
       static_cast<int>(rtcVbatMinMv),
       static_cast<int>(chargeState),
       static_cast<int>(vbatMv < VBAT_CRITICAL_MV));
  LOGLN(2, msg);
}

// -------------------------------------------------------------------------------------------------
// publishPreSleepVbat() – mesure juste AVANT l'extinction du modem
//
// Appelée depuis loop() avant modem_off() + esp_deep_sleep_start().
// trigger="pre_sleep" permet à Node-RED de distinguer cette série des mesures périodiques
// et de la stocker dans un champ DB séparé si souhaité.
// -------------------------------------------------------------------------------------------------
void publishPreSleepVbat()
{
  if (!mqtt.connected()) {
    return;
  }

  const int32_t vbatMv = readVbatMv();
  if (vbatMv < 2000 || vbatMv > 5000) {
    LOGF(1, "Pre-sleep Vbat hors plage ignorée: %d mV\n", static_cast<int>(vbatMv));
    return;
  }

  // Mise à jour des minimums (pre-sleep capture souvent le pire du cycle)
  if (vbatMv < cycleVbatMinMv) {
    cycleVbatMinMv = vbatMv;
  }
  if (vbatMv < rtcVbatMinMv) {
    rtcVbatMinMv = vbatMv;
  }

  const int32_t vbatDeltaMv = rtcVbatEmaMv - rtcVbatPrevEmaMv;

  // datetime : utiliser le dernier fix valide disponible
  const char *dt = (lastFix.valid && lastFix.datetime[0] != '\0')
                   ? lastFix.datetime
                   : "unknown";

  int8_t chargeState = 0;
  int8_t percent     = 0;
  int16_t modemMv    = 0;
  modem.getBattStats(chargeState, percent, modemMv);

  StaticJsonDocument<448> doc;
  doc["capteur"]           = matUuid;
  doc["datetime"]          = dt;
  doc["vbat_mv"]           = vbatMv;
  doc["vbat_ema_mv"]       = static_cast<int32_t>(rtcVbatEmaMv);
  doc["vbat_delta_mv"]     = vbatDeltaMv;
  doc["vbat_min_mv"]       = static_cast<int32_t>(rtcVbatMinMv);
  doc["vbat_min_cycle_mv"] = static_cast<int32_t>(cycleVbatMinMv);
  doc["boot_count"]        = static_cast<uint32_t>(bootCount);
  doc["charge_state"]      = chargeState;
  doc["pre_sleep"]         = true;
  doc["alert"]             = (vbatMv < VBAT_CRITICAL_MV);
  doc["trigger"]           = "pre_sleep";

  String msg;
  serializeJson(doc, msg);

  if (!mqtt.publish(mqttbat, msg.c_str())) {
    LOGF(1, "MQTT pre-sleep bat publish failed, state=%d\n", mqtt.state());
    return;
  }

  LOGLN(1, "Pre-sleep battery publish OK");
  LOGF(1, "  vbat=%d mV  ema=%d mV  delta=%+d mV  min_cycle=%d mV  min=%d mV  charge=%d  alert=%d\n",
       static_cast<int>(vbatMv),
       static_cast<int>(rtcVbatEmaMv),
       static_cast<int>(vbatDeltaMv),
       static_cast<int>(cycleVbatMinMv),
       static_cast<int>(rtcVbatMinMv),
       static_cast<int>(chargeState),
       static_cast<int>(vbatMv < VBAT_CRITICAL_MV));
  LOGLN(2, msg);
}

// -------------------------------------------------------------------------------------------------
// publishNtripConnectVbat() – mesure au moment du pic de courant LTE (connexion NTRIP)
//
// La connexion 4G initiale (reqRaw) tire jusqu'à 500 mA → c'est le moment où la tension
// LiPo est la plus basse du cycle. Cette mesure capture systématiquement le sag de démarrage,
// que publishPreSleepVbat() manquait dans 54% des cycles (V1 < V3 observé dans les données).
//
// Appelée depuis maintainNtrip() juste après la connexion réussie au caster.
// trigger="ntrip_connect" permet de filtrer cette série séparément en Node-RED.
// -------------------------------------------------------------------------------------------------
void publishNtripConnectVbat()
{
  if (!mqtt.connected()) {
    return;
  }

  const int32_t vbatMv = readVbatMv();
  if (vbatMv < 2000 || vbatMv > 5000) {
    LOGF(1, "NTRIP-connect Vbat hors plage: %d mV\n", static_cast<int>(vbatMv));
    return;
  }

  // Mise à jour des minimums – ce moment est souvent le pire du cycle
  if (vbatMv < cycleVbatMinMv) {
    cycleVbatMinMv = vbatMv;
  }
  if (vbatMv < rtcVbatMinMv) {
    rtcVbatMinMv = vbatMv;
  }

  const int32_t vbatDeltaMv = rtcVbatEmaMv - rtcVbatPrevEmaMv;

  const char *dt = (lastFix.valid && lastFix.datetime[0] != '\0')
                   ? lastFix.datetime
                   : "unknown";

  int8_t chargeState = 0;
  int8_t percent     = 0;
  int16_t modemMv    = 0;
  modem.getBattStats(chargeState, percent, modemMv);

  StaticJsonDocument<448> doc;
  doc["capteur"]           = matUuid;
  doc["datetime"]          = dt;
  doc["vbat_mv"]           = vbatMv;
  doc["vbat_ema_mv"]       = static_cast<int32_t>(rtcVbatEmaMv);
  doc["vbat_delta_mv"]     = vbatDeltaMv;
  doc["vbat_min_mv"]       = static_cast<int32_t>(rtcVbatMinMv);
  doc["vbat_min_cycle_mv"] = static_cast<int32_t>(cycleVbatMinMv);
  doc["boot_count"]        = static_cast<uint32_t>(bootCount);
  doc["charge_state"]      = chargeState;
  doc["pre_sleep"]         = false;
  doc["alert"]             = (vbatMv < VBAT_CRITICAL_MV);
  doc["trigger"]           = "ntrip_connect";

  String msg;
  serializeJson(doc, msg);

  if (!mqtt.publish(mqttbat, msg.c_str())) {
    LOGF(1, "MQTT ntrip-connect bat publish failed, state=%d\n", mqtt.state());
    return;
  }

  LOGLN(1, "NTRIP-connect battery publish OK");
  LOGF(1, "  vbat=%d mV  ema=%d mV  delta=%+d mV  min_cycle=%d mV  alert=%d\n",
       static_cast<int>(vbatMv),
       static_cast<int>(rtcVbatEmaMv),
       static_cast<int>(vbatDeltaMv),
       static_cast<int>(cycleVbatMinMv),
       static_cast<int>(vbatMv < VBAT_CRITICAL_MV));
  LOGLN(2, msg);
}

int mapPositionTypeToFix(const String &positionType)
{
  if (positionType == "NONE") return 0;
  if (positionType == "PSRDIFF" || positionType == "SBAS") return 2;
  if (positionType == "SINGLE") return 3;
  if (positionType.indexOf("FLOAT") >= 0) return 4;
  if (positionType.indexOf("INT") >= 0) return 5;  // RTK fix entier : distingué de FLOAT(4)
  return 3;
}

int mapPositionTypeToCarrier(const String &positionType)
{
  if (positionType.indexOf("FLOAT") >= 0) return 1;
  if (positionType.indexOf("INT") >= 0) return 2;
  return 0;
}

// -------------------------------------------------------------------------------------------------
// NTRIP  (unchanged – runs on Core 1 with the rest of network code)
// -------------------------------------------------------------------------------------------------

void maintainNtrip()
{
  if (ntripClient.connected()) {
    return;
  }

  LOGF(1, "Opening socket to %s\n", casterHost);

  if (ntripClient.reqRaw(casterHost, casterPort, mountPoint, casterUser, casterUserPW)) {
    const unsigned long connectedAt = millis();

    LOGLN(1, "Connected to NTRIP caster");
    lastReceivedRTCM_ms = connectedAt;

    // Mesure batterie immédiate : pic de courant LTE au moment de la connexion
    publishNtripConnectVbat();

    // Démarre la fenêtre RTK au vrai moment où le flux NTRIP devient disponible
    if (lastState == 0) {
      lastState = connectedAt;
      LOGF(1, "RTK acquisition window started at NTRIP connect: %lu\n", connectedAt);
    }

    return;
  }

  LOGLN(1, "Could not connect to NTRIP caster. Retrying in 5 seconds.");
  feedTaskWatchdog();
  delay(5000);
  feedTaskWatchdog();
}

void processNtripStream()
{
  if (!ntripClient.connected()) {
    return;
  }

  uint8_t rtcmData[2048];
  size_t rtcmCount = 0;

  while (ntripClient.available()) {
    int c = ntripClient.read();
    if (c < 0) {
      break;
    }
    rtcmData[rtcmCount++] = static_cast<uint8_t>(c);
    if (rtcmCount == sizeof(rtcmData)) {
      break;
    }
  }

  if (rtcmCount > 0) {
    lastReceivedRTCM_ms = millis();
    // GNSSSerial.write() is safe from Core 1 while Core 0 only reads it
    GNSSSerial.write(rtcmData, rtcmCount);
    feedTaskWatchdog();
    LOGF(1, "Forwarded %u RTCM bytes to UM980\n", rtcmCount);
  }

  if ((millis() - lastReceivedRTCM_ms) > maxTimeBeforeHangup_ms) {
    LOGLN(1, "RTCM timeout!");
    ntripClient.stop();
  }
}

// ── MODIFIED: reads lastGGASentence under ggaMutex ───────────────────────────
void sendPeriodicGGA()
{
  if (!transmitLocation) {
    return;
  }

  if (!ntripClient.connected()) {
    return;
  }

  if (millis() - lastGgaToCaster_ms < static_cast<unsigned long>(SEND_GGA_PERIOD * 1000UL)) {
    return;
  }

  // Take a local copy under the mutex so Core 0 can update lastGGASentence freely
  char ggaCopy[GNSS_LINE_BUFFER_SIZE];
  ggaCopy[0] = '\0';
  if (xSemaphoreTake(ggaMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    strncpy(ggaCopy, lastGGASentence.c_str(), sizeof(ggaCopy) - 1);
    ggaCopy[sizeof(ggaCopy) - 1] = '\0';
    xSemaphoreGive(ggaMutex);
  }

  if (ggaCopy[0] == '\0') {
    return;
  }

  if (ntripClient.sendGGA(ggaCopy)) {
    lastGgaToCaster_ms = millis();
    LOGF(2, "Pushed GGA to caster: %s", ggaCopy);
  }
}
// ────────────────────────────────────────────────────────────────────────────

// -------------------------------------------------------------------------------------------------
// Deep sleep / battery
// -------------------------------------------------------------------------------------------------

void setupBatteryCalibration()
{
  // GPIO35 est input-only sur l'ESP32 : pas de pinMode() nécessaire.
  // L'atténuation 11dB couvre 0-3.3V → parfait pour VBAT/2 (max ~2.1V à 4.2V Li-Ion).
  analogSetPinAttenuation(ADC_PIN, ADC_11db);

  const esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
      ADC_UNIT_1,
      ADC_ATTEN_DB_11,
      ADC_WIDTH_BIT_12,
      1100,
      &adcChars);

  adcCalibrated = true;

  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    LOGF(1, "ADC cal: eFuse Vref=%u mV (GPIO%d, diviseur ×%d)\n",
         adcChars.vref, ADC_PIN, VBAT_ADC_DIVIDER_RATIO);
    vref = adcChars.vref;
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    LOGF(1, "ADC cal: Two Point coeff_a=%u coeff_b=%u (GPIO%d, diviseur ×%d)\n",
         adcChars.coeff_a, adcChars.coeff_b, ADC_PIN, VBAT_ADC_DIVIDER_RATIO);
  } else {
    LOGF(1, "ADC cal: Default Vref 1100mV (GPIO%d, diviseur ×%d)\n",
         ADC_PIN, VBAT_ADC_DIVIDER_RATIO);
  }

  // Lecture de vérification immédiate au boot pour détecter un problème de diviseur
  const int32_t bootVbat = readVbatMv();
  LOGF(1, "ADC check: Vbat@boot=%d mV (attendu 3600-4250 mV pour 18650 nominal)\n",
       static_cast<int>(bootVbat));
  if (bootVbat < 2000 || bootVbat > 5000) {
    LOGLN(1, "ADC WARN: tension hors plage – vérifier diviseur sur GPIO35");
  }
}

void setupDeepSleep()
{
  LOGF(1, "SETUP - DEEPSLEEP State : %d\n", DEEP_SLEEP_ACTIVATED);

  if (DEEP_SLEEP_ACTIVATED) {
    LOGF(1, "SETUP - Sleep mode configured to : %d seconds\n", TIME_TO_SLEEP);
    esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(TIME_TO_SLEEP) * uS_TO_S_FACTOR);
    LOGF(1, "SETUP - GNSS acquisition period configured to : %d seconds\n", RTK_ACQUISITION_PERIOD);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_2, HIGH);
  } else {
    LOGLN(1, "SETUP - DeepSleep mode disactivated");
  }
}

void print_wakeup_reason()
{
  LOGLN(1, "-----------------");
  LOGLN(1, " - WAKEUP REASON ");
  ++bootCount;
  LOGF(1, " - Boot count = %u\n", static_cast<unsigned>(bootCount));
  LOGF(1, " - Consecutive failure cycles = %u\n", static_cast<unsigned>(consecutiveFailureCount));
  esp_sleep_wakeup_cause_t source_reveil = esp_sleep_get_wakeup_cause();

  switch (source_reveil) {
    case ESP_SLEEP_WAKEUP_EXT0:
      LOGLN(1, "Réveil causé par un signal externe avec RTC_IO");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      LOGLN(1, "Réveil causé par un signal externe avec RTC_CNTL");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      LOGLN(1, "Réveil causé par un timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      LOGLN(1, "Réveil causé par un touchpad");
      break;
    default:
      LOGF(1, "Réveil pas causé par le Deep Sleep: %d\n", source_reveil);
      // Boot froid (power-on ou reset manuel) : réinitialiser l'historique batterie.
      rtcVbatMinMv     = 9999;
      rtcVbatEmaMv     = 0;
      rtcVbatPrevEmaMv = 0;
      rtcVbatEmaInit   = false;
      break;
  }
}

// -------------------------------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------------------------------

int splitCsv(char *str, char **tokens, int maxTokens)
{
  int count = 0;
  char *saveptr = nullptr;
  char *token = strtok_r(str, ",", &saveptr);
  while (token != nullptr && count < maxTokens) {
    tokens[count++] = token;
    token = strtok_r(nullptr, ",", &saveptr);
  }
  return count;
}

bool gpsWeekTowToUtcString(uint16_t gpsWeek, uint32_t towMs, int leapSeconds, char *out, size_t outSize)
{
  constexpr int64_t gpsEpochUnix = 315964800LL; // 1980-01-06T00:00:00Z
  int64_t utcMs = static_cast<int64_t>(gpsWeek) * 604800000LL + static_cast<int64_t>(towMs) - static_cast<int64_t>(leapSeconds) * 1000LL;
  int64_t unixMs = gpsEpochUnix * 1000LL + utcMs;
  time_t unixSec = static_cast<time_t>(unixMs / 1000LL);
  int milli = static_cast<int>(unixMs % 1000LL);
  if (milli < 0) {
    milli += 1000;
    unixSec -= 1;
  }

  struct tm utcTime;
  gmtime_r(&unixSec, &utcTime);
  snprintf(out,
           outSize,
           "%04d-%02d-%02d %02d:%02d:%02d.%03d",
           utcTime.tm_year + 1900,
           utcTime.tm_mon + 1,
           utcTime.tm_mday,
           utcTime.tm_hour,
           utcTime.tm_min,
           utcTime.tm_sec,
           milli);
  return true;
}