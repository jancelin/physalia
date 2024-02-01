//Your WiFi credentials
//const char ssid[] = "blablabla";
//const char password[] = "12345678";

//MQTT connexion
const char* mqttServer = "mavi-mqtt.centipede.fr";
const int mqttPort = 8090;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtttopic = "test/mavi";

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
