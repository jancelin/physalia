//Your WiFi credentials
const char ssid[] = "WifiRaspi";
const char password[] = "wifiraspi";

//MQTT connexion
const char* mqttServer = "172.24.1.1";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtttopic = "buoy/mavi";

//MQTT connexion
// const char* mqttServer = "mavi-mqtt.centipede.fr";
// const int mqttPort = 8883;
// const char* mqttUser = "";
// const char* mqttPassword = "";
//const char* mqtttopic = "buoy/mavi";

//material uuid
const char matUuid[] = "'oasis_";

//Centipede works well and is free
const char casterHost[] = "caster.centipede.fr";
const uint16_t casterPort = 2101;
const char casterUser[] = "esp32";
const char casterUserPW[] = "centipede";
const char mountPoint[] = "SLP"; //The mount point you want to get data from

//Send gga to caster
const bool transmitLocation = false;
