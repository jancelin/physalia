//Your WiFi credentials
//const char ssid[] = "blablabla";
//const char password[] = "12345678";

// HARDWARE CONNECTION
#define pin_GNSS 33     // ANALOG PIN 33 ( Relais 1 )
#define pin_GSM 32        // ANALOG PIN 33 ( Relais 2 )
#define LDR 2  // composante photoresistor sur la pin GPI02

// DEEP SLEEP CONFIGURATION
#define uS_TO_S_FACTOR 1000000

int TIME_TO_SLEEP = 180; // temps de repos en deepsleep.
int RTK_ACQUISITION_PERIOD = 120; // Temps pendant lequel on doit capter de la donnée en RTK ( secondes )
int RTK_MAX_RESEARCH = 120; // Temps max pendant lequel le dispositif recherche du RTK ( seconds )

RTC_DATA_ATTR int lastPeriodRecord = 0;


// BAT
int BAT_PERIOD = 10;

//MQTT connexion
const char* mqttServer = "mavi-mqtt.centipede.fr";
const int mqttPort = 8090;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtttopic = "buoy/mavi";
const char* mqttbat = "buoy/bat";

//material uuid
const char matUuid[] = "'TESTv3'";

//Centipede works well and is free
const char casterHost[] = "caster.centipede.fr";
const uint16_t casterPort = 2101 ;
const char casterUser[] = "hello";
const char casterUserPW[] = "mavi";
const char mountPoint[] = "CT"; //The mount point you want to get data from

//Send gga to caster
const bool transmitLocation = false;
