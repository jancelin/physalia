// Your GPRS credentials (leave empty, if not needed)
const char apn[]      = "sl2sfr"; // APN (example: internet.vodafone.pt) use https://wiki.apnchanger.org
const char gprsUser[] = ""; // GPRS User
const char gprsPass[] = ""; // GPRS Password

// SIM card PIN (leave empty, if not defined)
const char simPIN[]   = "0000"; 

//Your WiFi credentials
const char ssid[] = "GeoPoppy_Pi3x";
const char password[] = "geopoppy";

//MQTT connexion
const char* mqttServer = "147.100.179.215";
const int mqttPort = 8090;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtttopic = "buoy/gnss";

//material uuid
const char matUuid[] = "'ESP32_";

//Centipede works well and is free
const char casterHost[] = "crtk.fr";
const uint16_t casterPort = 9999;
const char casterUser[] = "centipede32";
const char casterUserPW[] = "centipede32";
const char mountPoint[] = "ME"; //The mount point you want to get data from

//Send gga to caster
const bool transmitLocation = true;
