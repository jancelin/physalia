#include "secrets.h"

// =============================================================================================
// secrets.cpp – Définitions uniques des paramètres déclarés dans secrets.h
// Ne pas modifier ce fichier pour configurer la bouée : éditer uniquement secrets.h.
// =============================================================================================

const char matUuid[] = PH_MAT_UUID;

const char apn[]      = PH_MODEM_APN;
const char gprsUser[] = PH_MODEM_GPRS_USER;
const char gprsPass[] = PH_MODEM_GPRS_PASS;

const char mountPoint[]   = PH_MOUNT_POINT;
const char casterHost[]   = PH_CASTER_HOST;
const uint16_t casterPort = PH_CASTER_PORT;
const char casterUser[]   = PH_CASTER_USER;
const char casterUserPW[] = PH_CASTER_PASSWORD;
const bool transmitLocation = PH_TRANSMIT_LOCATION;
const int  SEND_GGA_PERIOD  = PH_SEND_GGA_PERIOD;
const bool SEND_GGA_ONCE    = PH_SEND_GGA_ONCE;

const char* mqttServer   = PH_MQTT_SERVER;
const int   mqttPort     = PH_MQTT_PORT;
const char* mqttUser     = PH_MQTT_USER;
const char* mqttPassword = PH_MQTT_PASSWORD;
const char* mqtttopic    = PH_MQTT_TOPIC_GEO;
const char* mqttbat      = PH_MQTT_TOPIC_BAT;
const char* mqttstatus   = PH_MQTT_TOPIC_STATUS;
const size_t MQTT_BUFFER_SIZE = PH_MQTT_BUFFER_SIZE;
const uint16_t GEO_BATCH_CHUNK_RECORDS = PH_GEO_BATCH_CHUNK_RECORDS;

bool DEEP_SLEEP_ACTIVATED   = PH_DEEP_SLEEP_ACTIVATED;
int  TIME_TO_SLEEP          = PH_TIME_TO_SLEEP;
int  RTK_ACQUISITION_PERIOD = PH_RTK_ACQUISITION_PERIOD;
int  RTK_MAX_RESEARCH       = PH_RTK_MAX_RESEARCH;
RTC_DATA_ATTR int lastPeriodRecord = PH_LAST_PERIOD_RECORD_INIT;

int ACQUISION_PERIOD_4G   = PH_ACQUISION_PERIOD_4G;
int ACQUISION_PERIOD_MQTT = PH_ACQUISION_PERIOD_MQTT;
int ACQUISION_PERIOD_GNSS = PH_ACQUISION_PERIOD_GNSS;
const uint32_t NO_GGA_TIMEOUT_MS      = PH_NO_GGA_TIMEOUT_MS;
const uint32_t NO_RTCM_TIMEOUT_MS     = PH_NO_RTCM_TIMEOUT_MS;
const uint32_t NTRIP_RETRY_DELAY_MS   = PH_NTRIP_RETRY_DELAY_MS;

int BAT_PERIOD = PH_BAT_PERIOD;
const int VBAT_CRITICAL_MV = PH_VBAT_CRITICAL_MV;
const int VBAT_ADC_DIVIDER_RATIO = PH_VBAT_ADC_DIVIDER_RATIO;

const uint32_t MODEM_POWEROFF_TIMEOUT_MS = PH_MODEM_POWEROFF_TIMEOUT_MS;
const uint32_t PERIODIC_MODEM_RESET_CYCLES = PH_PERIODIC_MODEM_RESET_CYCLES;

int LOG_LEVEL = PH_LOG_LEVEL;
