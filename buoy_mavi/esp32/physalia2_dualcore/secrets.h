#pragma once

#include <Arduino.h>
#include <stdint.h>

/*
=============================================================================================
PHYSALIA
  - Plateforme HYdrographique pour la Surveillance Altimétrique du LIttoral
  - Hydrographic Platform for Altimetric Monitoring of the Coastal Zone

* Souce: https://github.com/jancelin/physalia
* License: GNU Affero General Public License v3.0

* Parameter file for the buoy_sim7600_rtk.ino
=============================================================================================
*/
//Material uuid
const char matUuid[] = "'Physalia2'";

//RTK connection
const char mountPoint[] = "NEAR"; //The mount point you want to get data from
const char casterHost[] = "crtk.net";
const uint16_t casterPort = 2101 ;
const char casterUser[] = "mavi";
const char casterUserPW[] = "mavi";
const bool transmitLocation = true; //Send gga to caster
const int SEND_GGA_PERIOD = 3; 

//MQTT connexion
const char* mqttServer = "mavi-mqtt.centipede.fr";
const int mqttPort = 8090;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtttopic = "buoy/physalia2-geo";
const char* mqttbat = "buoy/physalia2-bat";

// DEEP SLEEP CONFIGURATION
bool DEEP_SLEEP_ACTIVATED = true;     // True = DeepSleep sinon DeepSleep ( off ) captation en continue
int TIME_TO_SLEEP = 120; // temps de repos en deepsleep.
int RTK_ACQUISITION_PERIOD = 20; //120; // Temps ( en seconde ) pendant lequel on doit capter de la donnée en RTK ( secondes )
int RTK_MAX_RESEARCH = 60; // Temps max pendant lequel le dispositif recherche du RTK ( secondes )
#define uS_TO_S_FACTOR 1000000
RTC_DATA_ATTR int lastPeriodRecord = 0;
int ACQUISION_PERIOD_4G = 120; // Temps ( en seconde ) pendant lequel on va chercher le network 4G avant de faire un deepsleep( TIME_TO_SLEEP )
int ACQUISION_PERIOD_MQTT = 8000; // Temps d'acquisition pendant lequel on va chercher le serveur mqtt
int ACQUISION_PERIOD_GNSS = 5000; // Temps d'acquisition pendant lequel on va chercher le serveur mqtt

// BAT
int BAT_PERIOD = 15;    // Interval pour envoi de l'état de la batterie (en seconde )

// LOG LEVEL  –  0 = rien  |  1 = minimal (état modules, erreurs)  |  2 = complet (payloads, RTCM, GGA)
int LOG_LEVEL = 1;

//Your WiFi credentials
//const char ssid[] = "blabla";
//const char password[] = "12345678";