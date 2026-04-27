#pragma once

#include <Arduino.h>
#include <stdint.h>

/*
=============================================================================================
PHYSALIA – fichier de configuration utilisateur
* ÉDITER UNIQUEMENT LES VALEURS PH_* DANS CE FICHIER.
* Ne pas définir de variable globale directement ici : ce header est inclus par plusieurs .cpp.
* Les variables globales sont déclarées en extern ci-dessous et définies une seule fois dans secrets.cpp.
=============================================================================================
*/

// =============================================================================================
// 1) VALEURS MODIFIABLES PAR L'UTILISATEUR
// =============================================================================================

// --- Identifiant matériel ---
#define PH_MAT_UUID "'Physalia2'"

// --- Connexion cellulaire / APN ---
#define PH_MODEM_APN           "sl2sfr"
#define PH_MODEM_GPRS_USER     ""
#define PH_MODEM_GPRS_PASS     ""

// --- Connexion NTRIP ---
#define PH_MOUNT_POINT        "NEAR"
#define PH_CASTER_HOST        "crtk.net"
#define PH_CASTER_PORT        2101
#define PH_CASTER_USER        "mavi"
#define PH_CASTER_PASSWORD    "mavi"
#define PH_TRANSMIT_LOCATION  true
#define PH_SEND_GGA_PERIOD    3          // secondes ; secours uniquement, l'envoi nominal est one-shot
#define PH_SEND_GGA_ONCE      true       // NEAR : 1 GGA dès connexion caster, puis pas de spam GGA

// --- Connexion MQTT ---
#define PH_MQTT_SERVER        "mavi-mqtt.centipede.fr"
#define PH_MQTT_PORT          8090
#define PH_MQTT_USER          ""
#define PH_MQTT_PASSWORD      ""
#define PH_MQTT_TOPIC_GEO     "buoy/physalia2-geo"
#define PH_MQTT_TOPIC_BAT     "buoy/physalia2-bat"
#define PH_MQTT_TOPIC_STATUS  "buoy/physalia2-status"
#define PH_MQTT_BUFFER_SIZE   2048       // publications unitaires compatibles Node-RED/PostgreSQL.

// --- Batching mesure ---
#define PH_FIX_BATCH_MAX_RECORDS       300   // 60 s à 5 Hz. En cas de dépassement, les plus anciens sont supprimés.
#define PH_GEO_BATCH_CHUNK_RECORDS       1   // 1 = publication unitaire fiable, compatible ancien flux Node-RED.
#define PH_BATTERY_BATCH_MAX_RECORDS     8   // boot, LTE/intermédiaire, pre_sleep, diagnostics éventuels

// --- Deep-sleep / acquisition ---
#define PH_DEEP_SLEEP_ACTIVATED    true
#define PH_TIME_TO_SLEEP           600   // secondes de sommeil nominal
#define PH_RTK_ACQUISITION_PERIOD  5     // secondes après RTK FIX confirmé
#define PH_RTK_MAX_RESEARCH        60    // secondes max entre connexion NTRIP et RTK FIX
#define uS_TO_S_FACTOR             1000000ULL
#define PH_LAST_PERIOD_RECORD_INIT 0

// --- Timeouts réseau / GNSS ---
// PH_ACQUISION_PERIOD_4G en SECONDES (le code fait * 1000UL pour millis()).
#define PH_ACQUISION_PERIOD_4G     45     // 120 s était trop coûteux pour 1×18650 ; 45 s = stratégie énergie
#define PH_ACQUISION_PERIOD_MQTT   8000   // timeout MQTT post-acquisition (millisecondes)
#define PH_ACQUISION_PERIOD_GNSS   12000  // délai UM980 pour temps GPS/GGA/PVTSLNA (millisecondes)
#define PH_NO_GGA_TIMEOUT_MS       25000UL
#define PH_NO_RTCM_TIMEOUT_MS      15000UL
#define PH_NTRIP_RETRY_DELAY_MS     3000UL

// --- Batterie ---
// BAT_PERIOD contrôle l'échantillon intermédiaire local ; publication groupée en fin de cycle.
#define PH_BAT_PERIOD              8
#define PH_VBAT_CRITICAL_MV        3600
#define PH_VBAT_ADC_DIVIDER_RATIO  2

// --- Modem ---
#define PH_MODEM_POWEROFF_TIMEOUT_MS   5000UL
#define PH_PERIODIC_MODEM_RESET_CYCLES 144UL

// --- Log : 0=silence | 1=minimal | 2=complet ---
#define PH_LOG_LEVEL 1

// =============================================================================================
// 2) DÉCLARATIONS GLOBALES — NE PAS MODIFIER
// =============================================================================================

extern const char matUuid[];

extern const char apn[];
extern const char gprsUser[];
extern const char gprsPass[];

extern const char mountPoint[];
extern const char casterHost[];
extern const uint16_t casterPort;
extern const char casterUser[];
extern const char casterUserPW[];
extern const bool transmitLocation;
extern const int SEND_GGA_PERIOD;
extern const bool SEND_GGA_ONCE;

extern const char* mqttServer;
extern const int mqttPort;
extern const char* mqttUser;
extern const char* mqttPassword;
extern const char* mqtttopic;
extern const char* mqttbat;
extern const char* mqttstatus;
extern const size_t MQTT_BUFFER_SIZE;
extern const uint16_t GEO_BATCH_CHUNK_RECORDS;

extern bool DEEP_SLEEP_ACTIVATED;
extern int TIME_TO_SLEEP;
extern int RTK_ACQUISITION_PERIOD;
extern int RTK_MAX_RESEARCH;
extern int lastPeriodRecord;

extern int ACQUISION_PERIOD_4G;
extern int ACQUISION_PERIOD_MQTT;
extern int ACQUISION_PERIOD_GNSS;
extern const uint32_t NO_GGA_TIMEOUT_MS;
extern const uint32_t NO_RTCM_TIMEOUT_MS;
extern const uint32_t NTRIP_RETRY_DELAY_MS;

extern int BAT_PERIOD;
extern const int VBAT_CRITICAL_MV;
extern const int VBAT_ADC_DIVIDER_RATIO;

extern const uint32_t MODEM_POWEROFF_TIMEOUT_MS;
extern const uint32_t PERIODIC_MODEM_RESET_CYCLES;

extern int LOG_LEVEL;
