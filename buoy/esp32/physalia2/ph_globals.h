#pragma once
// =============================================================================
// ph_globals.h – Déclarations extern de toutes les variables globales.
// Les DÉFINITIONS sont dans physalia2_dualcore.ino.
// =============================================================================

#include "ph_config.h"
#include "secrets.h"
#include "NTRIPClient.h"

struct PvtslnData {
  bool   valid       = false;
  char   datetime[32] = {0};
  String bestposType;
  double lat        = 0.0;
  double lon        = 0.0;
  double altMSL     = 0.0;
  double undulation = 0.0;
  double ellipsoid  = 0.0;
  double velNorth   = 0.0;
  double velEast    = 0.0;
  double velGround  = 0.0;
  float  hgtStd     = 0.0f;
  float  latStd     = 0.0f;
  float  lonStd     = 0.0f;
  float  diffAge    = 0.0f;
  String headingType;
  float  headingLength = 0.0f;
  float  headingDeg    = 0.0f;
  float  headingPitch  = 0.0f;
  float  gdop = 0.0f;
  float  pdop = 0.0f;
  float  hdop = 0.0f;
  float  htdop = 0.0f;
  float  tdop  = 0.0f;
  uint8_t bestposTrackedSvs  = 0;
  uint8_t bestposSolnSvs     = 0;
  uint8_t psrposTrackedSvs   = 0;
  uint8_t psrposSolnSvs      = 0;
  uint8_t headingTrackedSvs  = 0;
  uint8_t headingSolnSvs     = 0;
  uint8_t headingGgl1        = 0;
  uint8_t headingGgl1l2      = 0;
};

struct BatterySample {
  bool valid = false;
  char stage[16] = {0};
  char datetime[32] = {0};
  uint32_t tMs = 0;
  int32_t vbatMv = 0;
};

extern TinyGsm         modem;
extern TinyGsmClient   mqttTransport;
extern TinyGsmClient   ntripTransport;
extern PubSubClient    mqtt;
extern NTRIPClient     ntripClient;
extern HardwareSerial  GNSSSerial;

extern unsigned long bootStarted_ms;
extern unsigned long lastReconnectAttempt;
extern unsigned long lastReceivedRTCM_ms;
extern unsigned long lastGgaToCaster_ms;
extern unsigned long lastPvtsln_ms;
extern unsigned long lastState;
extern unsigned long firstGga_ms;
extern unsigned long lastNtripAttempt_ms;
extern unsigned long ntripConnected_ms;
extern unsigned long firstRtcm_ms;
extern unsigned long firstFloat_ms;
extern unsigned long firstFix_ms;
extern unsigned long rtkFixStart_ms;
extern bool ggaSentToCaster;
extern bool cycleClosed;

extern char   gnssLine[];
extern size_t gnssLineLen;
extern String lastGGASentence;

extern PvtslnData lastFix;
extern PvtslnData fixBatch[PH_FIX_BATCH_MAX_RECORDS];
extern uint16_t fixBatchCount;
extern uint32_t fixBatchDropped;

extern BatterySample batterySamples[PH_BATTERY_BATCH_MAX_RECORDS];
extern uint8_t batterySampleCount;

extern QueueHandle_t     pvtQueue;
extern SemaphoreHandle_t ggaMutex;

extern bool taskWdtEnabled;

extern uint32_t bootCount;
extern uint8_t  consecutiveFailureCount;
extern uint32_t cyclesSinceModemReset;
extern uint8_t  failureSleepCount;
extern bool     lastBootWasRestart;
extern uint32_t lastRestartTimestampMs;
extern uint32_t cycleId;

extern int   vref;
extern uint32_t timeStamp;
extern esp_adc_cal_characteristics_t adcChars;
extern bool  adcCalibrated;
extern int32_t rtcVbatMinMv;
extern int32_t rtcVbatEmaMv;
extern bool    rtcVbatEmaInit;
