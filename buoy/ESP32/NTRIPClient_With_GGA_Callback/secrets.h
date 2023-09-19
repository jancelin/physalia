//Your WiFi credentials
const char ssid[] = "buoy";
const char password[] = "Buoy_43210!";

//MQTT connexion
const char* mqttServer = "127.0.0.1";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtttopic = "buoy/gnss_test";

//material uuid
const char matUuid[] = "'Buoy_eps32_";

//Centipede works well and is free
const char casterHost[] = "caster.centipede.fr";
const uint16_t casterPort = 2101;
const char casterUser[] = "centipede32"; //User must provide their own email address to use RTK2Go
const char casterUserPW[] = "centipede32";
const char mountPoint[] = "LIENSS"; //The mount point you want to get data from

//Send gga to caster
const bool transmitLocation = true;
