#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "GNSS.h"
#include "Modem.h"
#include "NTRIPClient.h"
#include "MQTTClient.h"
#include "DeepSleep.h"

GNSS gnss;
Modem modem;
NTRIPClient ntripClient;
MQTTClient mqttClient;
DeepSleep deepSleep;

void setup() {
  Serial.begin(115200);
  Serial.println("********************************");
  Serial.println("******** SETUP BEGIN ***********");
  Serial.println("********************************");

  gnss.setup();
  modem.setup();
  mqttClient.setup();
}

void loop() {
  gnss.process();
  modem.process();
  ntripClient.process();
  mqttClient.process();
  deepSleep.process();
}