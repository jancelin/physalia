//Your WiFi credentials
const char ssid[] = "buoy";
const char password[] = "Buoy_43210!";

//MQTT connexion
const char* mqttServer = "mavi-mqtt.centipede.fr";
const int mqttPort = 8090;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtttopic = "buoy/gnss_test";

//material uuid
const char matUuid[] = "'brou2_";

//Centipede works well and is free
const char casterHost[] = "caster.centipede.fr";
const uint16_t casterPort = 2101;
const char casterUser[] = "esp32";
const char casterUserPW[] = "centipede";
const char mountPoint[] = "SLP"; //The mount point you want to get data from

//Send gga to caster
const bool transmitLocation = false;
