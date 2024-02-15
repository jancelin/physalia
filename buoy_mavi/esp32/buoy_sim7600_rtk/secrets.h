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
const char matUuid[] = "'TESTv3'";

//RTK connection
const char mountPoint[] = "LIENSS"; //The mount point you want to get data from
const char casterHost[] = "caster.centipede.fr";
const uint16_t casterPort = 2101 ;
const char casterUser[] = "mavi";
const char casterUserPW[] = "mavi";
const bool transmitLocation = false; //Send gga to caster

//MQTT connexion
const char* mqttServer = "mavi-mqtt.centipede.fr";
const int mqttPort = 8090;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtttopic = "buoy/mavi";
const char* mqttbat = "buoy/bat";

// GNSS acquisition Frequency ( Hz )
int GNSS_FREQ = 2;

// DEEP SLEEP CONFIGURATION
bool DEEP_SLEEP_ACTIVATED = true;     // True = DeepSleep sinon DeepSleep ( off ) captation en continue
int TIME_TO_SLEEP = 30; // temps de repos en deepsleep.
int RTK_ACQUISITION_PERIOD = 120; // Temps pendant lequel on doit capter de la donnée en RTK ( secondes )
int RTK_MAX_RESEARCH = 120; // Temps max pendant lequel le dispositif recherche du RTK ( secondes )
#define uS_TO_S_FACTOR 1000000
RTC_DATA_ATTR int lastPeriodRecord = 0;
int ACQUISION_PERIOD_4G = 120; // Temps ( en seconde ) pendant lequel on va chercher le network 4G avant de faire un deepsleep( TIME_TO_SLEEP )

// BAT
int BAT_PERIOD = 10;    // Interval pour envoi de l'état de la batterie (en seconde )

//Your WiFi credentials
//const char ssid[] = "blabla";
//const char password[] = "12345678";