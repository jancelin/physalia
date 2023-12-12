//Your WiFi credentials OASIS
// const char ssid[] = "WifiRaspi";
// const char password[] = "wifiraspi";
const char ssid[] = "ici";
const char password[] = "12345678";

//MQTT connexion OASIS
// const char* mqttServer = "172.24.1.1";
// const int mqttPort = 1883;
// const char* mqttUser = "";
// const char* mqttPassword = "";
// const char* mqtttopic = "buoy/mavi";

//MQTT St Medard
const char* mqttServer = "mavi-mqtt.centipede.fr";
const int mqttPort = 8090;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtttopic = "TEST_BUOY/ROMAIN";

//MQTT connexion
// const char* mqttServer = "mavi-mqtt.centipede.fr";
// const int mqttPort = 8883;
// const char* mqttUser = "";
// const char* mqttPassword = "";
//const char* mqtttopic = "buoy/mavi";

//material uuid
const char matUuid[] = "'oasis_romain";

//Centipede works well and is free
const char casterHost[] = "caster.centipede.fr";
const uint16_t casterPort = 2101;
const char casterUser[] = "esp32";
const char casterUserPW[] = "centipede";
const char mountPoint[] = "SLP"; //The mount point you want to get data from

//Send gga to caster
const bool transmitLocation = false;
