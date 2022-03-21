// Your GPRS credentials (leave empty, if not needed)
const char apn[]      = "sl2sfr"; // APN (example: internet.vodafone.pt) use https://wiki.apnchanger.org
const char gprsUser[] = ""; // GPRS User
const char gprsPass[] = ""; // GPRS Password

// SIM card PIN (leave empty, if not defined)
const char simPIN[]   = "0000"; 

//Your WiFi credentials
const char ssid[] = "BUOY";
const char password[] = "12345678";

//MQTT connexion
const char* mqttServer = "127.0.0.1";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtttopic = "buoy/gnss";

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
