//Your WiFi credentials
const char ssid[] = "buoy";
const char password[] = "12345678";


//MQTT connexion
const char* mqttServer = "147.100.179.32";
const int mqttPort = 8090;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtttopic = "buoy/gnss_test";

//material uuid
const char matUuid[] = "'Physalita1_";

//Centipede works well and is free
const char casterHost[] = "caster.centipede.fr";
const uint16_t casterPort = 2101;
const char casterUser[] = "buoy_jancelin";
const char casterUserPW[] = "centipede";
const char mountPoint[] = "SLP"; //The mount point you want to get data from

//Send gga to caster
const bool transmitLocation = false;
