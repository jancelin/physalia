//Your WiFi credentials
const char ssid[] = "wifi_hotspot";
const char password[] = "password";

//MQTT connexion
const char* mqttServer = "xxx.xxx.xxx.xxx";
const int mqttPort = 8090;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtttopic = "buoy/gnss";

//material uuid
const char matUuid[] = "'ESP32_";

//Centipede works well and is free
const char casterHost[] = "caster.centipede.fr";
const uint16_t casterPort = 2101;
const char casterUser[] = "esp32";
const char casterUserPW[] = "centipede";
const char mountPoint[] = "SLP"; //The mount point you want to get data from

//Send gga to caster
const bool transmitLocation = false;
