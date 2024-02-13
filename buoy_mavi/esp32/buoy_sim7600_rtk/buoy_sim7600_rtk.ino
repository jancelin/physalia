/*
=============================================================================================
PHYSALIA
  - Plateforme HYdrographique pour la Surveillance Altimétrique du LIttoral
  - Hydrographic Platform for Altimetric Monitoring of the Coastal Zone

* Souce: https://github.com/jancelin/physalia
* By: INRAE / Julien Ancelin & Romain Tourte, Quentin Lesniack
* License: GNU Affero General Public License v3.0

* Object: 
  - Connect to 4g LTE network and get RTCM data from CentipedeRTK caster as a Client
  - Transmit Lat long positions & batteries level over MQTT with adjustable deep sleep settings

* Material:
  - ESP32 with Pcie + bat: Lilygo T-pcie  https://www.tinytronics.nl/shop/en/development-boards/microcontroller-boards/with-wi-fi/lilygo-t-pcie-v1.2-axp2101-esp32-wrover-16mb
  - LTE 4G: SIM7600g                      https://www.tinytronics.nl/shop/en/communication-and-signals/wireless/gps/modules/lilygo-ttgo-t-pcie-sim7600g-h-expansion-module
  - F9P: drotek DP0601                    https://store-drotek.com/891-rtk-zed-f9p-gnss.html
  - relay: 5V 1-channel high-active       https://www.tinytronics.nl/shop/en/switches/relays/5v-relay-1-channel-high-active
  - battery: Li-Po Battery 3.7V 2000mAh   https://www.tinytronics.nl/shop/en/power/batteries/li-po/li-po-accu-3.7v-2000mah
  
* GNSS code:
  By: SparkFun Electronics / Nathan Seidle & Paul Clark
  Date: January 13th, 2022
  License: MIT.
=============================================================================================
*/
#include "secrets.h"
#include <Wire.h>
#include <SPI.h>

// GNSS V+ SWITCH
#define pin_GNSS 32     // ANALOG PIN 33 ( Relais 1 )
// GSM SIM7600G
#define UART_BAUD               115200
#define PIN_TX                  27
#define PIN_RX                  26
#define PWR_PIN                 4
#define LED_PIN                 12
#define POWER_PIN               25
#define IND_PIN                 36
// BAT
#include <esp_adc_cal.h>
#define ADC_PIN     35
int vref = 1100;
uint32_t timeStamp = 0;

//GSM----------------------------
// need enough space in the buffer for the entire response
// else data will be lost (and the http library will fail).
#define TINY_GSM_RX_BUFFER 1024

// Select your modem:
#define TINY_GSM_MODEM_SIM7600

#include <SoftwareSerial.h>
#define SerialAT Serial1

// See all AT commands, if wanted
//#define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG Serial

// Define how you're planning to connect to the internet.
// This is only needed for this example, not in other code.
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""
// Your GPRS credentials, if any
const char apn[]      = "sl2sfr";
const char gprsUser[] = "";
const char gprsPass[] = "";
// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "YourSSID";
const char wifiPass[] = "YourWiFiPass";
#include <TinyGsmClient.h>

// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#define TINY_GSM_POWERDOWN true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#define TINY_GSM_POWERDOWN true
#endif

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif
TinyGsmClient mqttClient(modem,0);
TinyGsmClient ntripClient(modem,2);
//GSM---------------------------

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;

#include <ArduinoJson.h>
#include <PubSubClient.h>

//WiFiClient espClient;

//The ESP32 core has a built in base64 library but not every platform does
//We'll use an external lib if necessary.
#if defined(ARDUINO_ARCH_ESP32)
#include "base64.h" //Built-in ESP32 library
#else
#include <Base64.h> //nfriendly library from https://github.com/adamvr/arduino-base64, will work with any platform
#endif

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
PubSubClient mqtt(mqttClient); //MQTT
long lastReconnectAttempt = 0;

/* CONFIG PERIOD DE CAPTATION EN RTK*/
bool state_fix = false;
long nb_millisecond_recorded = 0;
long lastState = 0;
long lastNetworkAttemps = 0;
void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

boolean reconnect() {
  if (mqtt.connect(mqtttopic)) {
    // Once connected, publish an announcement...
    mqtt.publish(mqtttopic, matUuid);
    // ... and resubscribe
    mqtt.subscribe(mqtttopic);
  }
  return mqtt.connected();
}
//GNSS Global variables=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

unsigned long lastReceivedRTCM_ms = 0;          //5 RTCM messages take approximately ~300ms to arrive at 115200bps
const unsigned long maxTimeBeforeHangup_ms = 10000UL; //If we fail to get a complete RTCM frame after 10s, then disconnect from caster

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Callback: pushGPGGA will be called when new GPGGA NMEA data arrives
// See u-blox_structs.h for the full definition of NMEA_GGA_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setNMEAGPGGAcallback
//        /               _____  This _must_ be NMEA_GGA_data_t
//        |              /           _____ You can use any name you like for the struct
//        |              |          /
//        |              |          |
void pushGPGGA(NMEA_GGA_data_t *nmeaData)
{
  //Provide the caster with our current position as needed
  if ((ntripClient.connected() == true) && (transmitLocation == true))
  {
    Serial.print(F("Pushing GGA to server: "));
    Serial.print((const char *)nmeaData->nmea); // .nmea is printable (NULL-terminated) and already has \r\n on the end

    //Push our current GGA sentence to caster
    ntripClient.print((const char *)nmeaData->nmea);
  }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Callback: printPVTdata will be called when new NAV PVT data arrives
// See u-blox_structs.h for the full definition of UBX_NAV_PVT_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setAutoPVTcallback
//        /                  _____  This _must_ be UBX_NAV_PVT_data_t
//        |                 /               _____ You can use any name you like for the struct
//        |                 |              /
//        |                 |              |
void printPVTdata(UBX_NAV_PVT_data_t *ubxDataStruct)
{
  long now = millis();
  // allocate the memory for the document
  StaticJsonDocument<256> doc;
  // create an object
  JsonObject object = doc.to<JsonObject>();

  //doc["capteur"] = matUuid + WiFi.macAddress()+"'";//matUuid; // Print capteur uuid
  doc["capteur"] = matUuid; // Print capteur uuid

  uint16_t y = ubxDataStruct->year; // Print the year
  uint8_t mo = ubxDataStruct->month; // Print the year
  String mo1;
  if (mo < 10) {mo1 = "0"+ String(mo);} else { mo1 = String(mo);};
  uint8_t d = ubxDataStruct->day; // Print the year
  String d1;
    if (d < 10) {d1 = "0"+ String(d);} else { d1 = String(d);};
  uint8_t h = ubxDataStruct->hour; // Print the hours
  String h1;
    if (h < 10) {h1 = "0"+ String(h);} else { h1 = String(h);};
  uint8_t m = ubxDataStruct->min; // Print the minutes
  String m1;
    if (m < 10) {m1 = "0"+ String(m);} else { m1 = String(m);};
  uint8_t s = ubxDataStruct->sec; // Print the seconds
  String s1;
    if (s < 10) {s1 = "0"+ String(s);} else { s1 = String(s);};
  unsigned long millisecs = ubxDataStruct->iTOW % 1000; // Print the milliseconds
  String a = ":";
  String b = "-";
  String date1 = y+b+mo1+b+d1+" ";
  String time1 = h1 +a+ m1 +a+ s1 + "." + millisecs;
  object["datetime"] = "'"+date1+time1+"'"; // print date time for postgresql data injection.

  double latitude = ubxDataStruct->lat; // Print the latitude
  doc["lat"] = latitude / 10000000.0;

  double longitude = ubxDataStruct->lon; // Print the longitude
  doc["lon"] = longitude / 10000000.0;

  double elevation = ubxDataStruct->height; // Print the height above mean sea level
  doc["elv_m"] = elevation / 1000.0;

  double altitude = ubxDataStruct->hMSL; // Print the height above mean sea level
  doc["alt_m"] = altitude / 1000.0;

  uint8_t fixType = ubxDataStruct->fixType; // Print the fix type
  // 0 none/1 Dead reckoning/2 2d/3 3d/4 GNSS + Dead reckoning/ 5 time only
  doc["fix"] = fixType;

  uint8_t carrSoln = ubxDataStruct->flags.bits.carrSoln; // Print the carrier solution
  // 0 none/1 floating/ 2 Fixed
  doc["car"] = carrSoln;

  uint32_t hAcc = ubxDataStruct->hAcc; // Print the horizontal accuracy estimate
  doc["hacc_mm"] = hAcc;

  uint32_t vAcc = ubxDataStruct->vAcc; // Print the vertical accuracy estimate
  doc["vacc_mm"] = vAcc;

  uint8_t numSV = ubxDataStruct->numSV; // Print tle number of SVs used in nav solution
  doc["numsv"] = numSV;

  serializeJson(doc, Serial);
  Serial.println();

  //Send position to MQTT broker
  String output = "JSON = ";
  String msg;
  serializeJson(doc, msg);
  mqtt.publish(mqtttopic, msg.c_str());
  Serial.println("Message send");

  //BAT
  if (millis() - timeStamp > BAT_PERIOD*1000) {
    timeStamp = millis();
    uint16_t v = analogRead(ADC_PIN);
    float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    String voltage = "'"+date1+time1+"'" + ","+ matUuid + ","+String(battery_voltage);

    // When connecting USB, the battery detection will return 0,
    // because the adc detection circuit is disconnected when connecting USB
    Serial.println(voltage);
    if (battery_voltage == 0.00 ) {
        Serial.println("USB is connected, please disconnect USB.");
    }
    mqtt.publish(mqttbat, voltage.c_str());
    Serial.println("Message send");
  }

}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void setup()
{
  Serial.begin(115200);
  Serial.println("********************************");
  Serial.println("******** SETUP BEGIN ***********");
  Serial.println("********************************");
  
  Serial.println("SETUP - Init Relay");
  pinMode(pin_GNSS,OUTPUT);
  digitalWrite(pin_GNSS, LOW);

  // Deep sleep 
  //Affiche la source du reveil
  print_wakeup_reason();
  Serial.print("SETUP - DEEPSLEEP State : ");
  Serial.println(DEEP_SLEEP_ACTIVATED);
  if ( DEEP_SLEEP_ACTIVATED ) {
    Serial.println("SETUP - Sleep mode configured to : " + String(TIME_TO_SLEEP) + " seconds" );
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("SETUP - GNSS acquisition period configured to : " + String(RTK_ACQUISITION_PERIOD) + " seconds" );
    // Configuration de WakeUp avec une photorésistance. 
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_2, HIGH);
  } else {
    Serial.println("SETUP - DeepSleep mode disactivated");
  }

  //BAT----------------------------
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);    //Check type of calibration value used to characterize ADC
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
      Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
      vref = adc_chars.vref;
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
      Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
  } else {
      Serial.println("Default Vref: 1100mV");
  }

  //GSM-----------------------------
  delay(10);
  // Onboard LED light, it can be used freely
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN,LOW);

  // POWER_PIN : This pin controls the power supply of the SIM7600
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);

  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  delay(500);
  digitalWrite(PWR_PIN, LOW);
  // IND_PIN: It is connected to the SIM7600 status Pin,
  // through which you can know whether the module starts normally.
  pinMode(IND_PIN, INPUT);

  Serial.println("\nWait...");
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(3000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  DBG("Initializing modem...");
  modem.restart();
  // modem.init();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);
  #if TINY_GSM_USE_GPRS
    // Unlock your SIM card with a PIN if needed
    if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }
  #endif

  #if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
      // The XBee must run the gprsConnect function BEFORE waiting for network!
      modem.gprsConnect(apn, gprsUser, gprsPass);
  #endif

  Serial.print("Waiting for network...");
  int lastNetworkAttemps = millis();
  int now = millis(); 

  // Testing 4G connection during ACQUISION_PERIOD_4G ( second ), if not connected after that, DeepSleep is launched
  while(!modem.waitForNetwork() && ( now - lastNetworkAttemps < ACQUISION_PERIOD_4G ) ) {
  //if (!modem.waitForNetwork()) {
    Serial.println("fail to find network, waiting 10sec before retry");
    delay(10000);
    now = millis();
    //return;
}

if ( DEEP_SLEEP_ACTIVATED ) {
  if ( now - lastNetworkAttemps < ACQUISION_PERIOD_4G ) {
    Serial.println(" success");
  }
  else {
    Serial.println("Max period attempted to connect to 4G, DeepSleep activated");
    modem_off();
    Serial.println("Modem Off; waiting 2 sec");
    esp_deep_sleep_start();
  }
}

if (modem.isNetworkConnected()) { Serial.println("Network connected"); }

#if TINY_GSM_USE_GPRS
  // GPRS connection parameters are usually set after network registration
  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  if (modem.isGprsConnected()) { Serial.println("GPRS connected"); }
#endif
//GSM---------------------------------

//GNSS=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  Serial.println(F("NTRIP testing"));

  Wire.begin(); //Start I2C

  while (myGNSS.begin() == false) //Connect to the Ublox module using Wire port
  {
    Serial.println(F("u-blox GPS not detected at default I2C address. Please check wiring."));
    delay(2000);
  }
  Serial.println(F("u-blox module connected"));

  myGNSS.setI2COutput(COM_TYPE_UBX | COM_TYPE_NMEA);                                //Set the I2C port to output both NMEA and UBX messages
  myGNSS.setPortInput(COM_PORT_I2C, COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3); //Be sure RTCM3 input is enabled. UBX + RTCM3 is not a valid state.
  myGNSS.setDGNSSConfiguration(SFE_UBLOX_DGNSS_MODE_FIXED); // Set the differential mode - ambiguities are fixed whenever possible
  myGNSS.setNavigationFrequency(GNSS_FREQ); //Set output in Hz.

  // Set the Main Talker ID to "GP". The NMEA GGA messages will be GPGGA instead of GNGGA
  myGNSS.setMainTalkerID(SFE_UBLOX_MAIN_TALKER_ID_GP);
  myGNSS.setNMEAGPGGAcallbackPtr(&pushGPGGA); // Set up the callback for GPGGA
  myGNSS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_I2C, 60); // Tell the module to output GGA every 60 seconds
  myGNSS.setAutoPVTcallbackPtr(&printPVTdata); // Enable automatic NAV PVT messages with callback to printPVTdata so we can watch the carrier solution go to fixed
  //myGNSS.saveConfiguration(VAL_CFG_SUBSEC_IOPORT | VAL_CFG_SUBSEC_MSGCONF); //Optional: Save the ioPort and message settings to NVM

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(callback);

  while (!mqtt.connected()) {
    Serial.println("Connecting to MQTT...\n");
    //if (mqtt.connect("ESP32Client", mqttUser, mqttPassword )) {
    //add a uuid for each rover https://github.com/knolleary/pubsubclient/issues/372#issuecomment-352086415
    if (mqtt.connect(matUuid , mqttUser, mqttPassword )) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(mqtt.state());
      delay(1500);
      lastReconnectAttempt = 0;
    }
  }

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  while (Serial.available()) // Empty the serial buffer
    Serial.read();
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void loop()
{
  long now = millis();
  // Check if error loop 
  if ( lastState !=0 && now - lastState > RTK_ACQUISITION_PERIOD*1000*1.2 ){ 
      Serial.println("Error detected during period is done at " + String(now));
      ESP.restart();
  }

  myGNSS.checkUblox(); // Check for the arrival of new GNSS data and process it.
  myGNSS.checkCallbacks(); // Check if any GNSS callbacks are waiting to be processed.

  enum states // Use a 'state machine' to open and close the connection
  {
    open_connection,
    push_data_and_wait_for_keypress,
    close_connection,
    waiting_for_keypress
  };
  static states state = open_connection;

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  switch (state)
  {
    case open_connection:
      Serial.println(F("Connecting to the NTRIP caster..."));
      if (beginClient()) // Try to open the connection to the caster
      {
        Serial.println(F("Connected to the NTRIP caster! Press any key to disconnect..."));
        state = push_data_and_wait_for_keypress; // Move on
      }
      else
      {
        Serial.print(F("Could not connect to the caster. Trying again in 5 seconds."));
        for (int i = 0; i < 5; i++)
        {
          delay(1000);
          Serial.print(F("."));
        }
        Serial.println();
      }
      break;

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    case push_data_and_wait_for_keypress:
      // If the connection has dropped or timed out, or if the user has pressed a key
      if ((processConnection() == false) || (keyPressed()))
      {
        state = close_connection; // Move on
      }
      break;

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    case close_connection:
      Serial.println(F("Closing the connection to the NTRIP caster..."));
      closeConnection();
      Serial.println(F("Press any key to reconnect..."));
      state = waiting_for_keypress; // Move on
      break;

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    case waiting_for_keypress:
      // If the connection has dropped or timed out, or if the user has pressed a key
      //if (keyPressed())
      state = open_connection; // Move on
      break;
  }
  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  //MQTT
   now = millis();
  if (!mqtt.connected()) {
    
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected

    mqtt.loop();
  }


  //GSM-------------------------------
  // Make sure we're still registered on the network
  if (!modem.isNetworkConnected()) {
    lastNetworkAttemps = millis();
    Serial.println("LOOP - Network disconnected");

    // Testing 4G connection during ACQUISION_PERIOD_4G ( second ), if not connected after that, DeepSleep is launched
    while(!modem.waitForNetwork() && ( now - lastNetworkAttemps < ACQUISION_PERIOD_4G ) ) {
      Serial.println("LOOP - fail to find network, waiting 10sec before retry");
      delay(10000);
      now = millis();
    }

    if (modem.isNetworkConnected()) {
      Serial.println("LOOP - Network re-connected");
    } 
    if ( DEEP_SLEEP_ACTIVATED ) {
      if ( now - lastNetworkAttemps < ACQUISION_PERIOD_4G ) {
        Serial.println("LOOP - Network re-connected before max attempts");
      }
      else {
        Serial.println("LOOP - Max period attempted to connect to 4G, DeepSleep activated");
        modem_off();
        Serial.println("LOOP - Modem Off; waiting 2 sec");
        esp_deep_sleep_start();
      }
    }

  #if TINY_GSM_USE_GPRS
    // and make sure GPRS/EPS is still connected
    if (!modem.isGprsConnected()) {
      Serial.println("GPRS disconnected!");
      Serial.print(F("Connecting to "));
      Serial.print(apn);
      if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        Serial.println(" fail");
        delay(10000);
        return;
      }
      if (modem.isGprsConnected()) { Serial.println("GPRS reconnected"); }
    }
  #endif
    }

  //DeepSleep configuration
    if ( lastState == 0 ) {
        Serial.println("lastState == 0 Valued to " + String(now) );
        lastState = now;
    }
    
    if ( DEEP_SLEEP_ACTIVATED && lastState !=0 && now - lastState > RTK_ACQUISITION_PERIOD*1000 ){
        Serial.println("Record during period is done, we can sleep at " + String(now) + " during " + String(TIME_TO_SLEEP) + " seconds");
        Serial.println("ESP32 will wake up in " + String(TIME_TO_SLEEP) + " seconds");
        modem_off();
        Serial.println("Modem Off; waiting 2 sec");
        delay(2000);
        Serial.println("Good night ! ");
        esp_deep_sleep_start();
    }  
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Permet d'afficher la raison du réveil du DeepSleep
void print_wakeup_reason(){
   Serial.println("-----------------");
   Serial.println(" - WAKEUP REASON ");
   esp_sleep_wakeup_cause_t source_reveil;
   source_reveil = esp_sleep_get_wakeup_cause();

   switch(source_reveil){
      case ESP_SLEEP_WAKEUP_EXT0 : 
        Serial.println("Réveil causé par un signal externe avec RTC_IO"); 
        break;
      case ESP_SLEEP_WAKEUP_EXT1 : 
        Serial.println("Réveil causé par un signal externe avec RTC_CNTL"); 
        break;
      case ESP_SLEEP_WAKEUP_TIMER : 
        Serial.println("Réveil causé par un timer"); 
        break;
      case ESP_SLEEP_WAKEUP_TOUCHPAD : 
        Serial.println("Réveil causé par un touchpad"); 
        break;
      default : 
        Serial.printf("Réveil pas causé par le Deep Sleep: %d\n",source_reveil); 
        break;
   }
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//Connect to NTRIP Caster. Return true is connection is successful.
bool beginClient()
{
  Serial.print(F("Opening socket to "));
  Serial.println(casterHost);

  if (ntripClient.connect(casterHost, casterPort) == false) //Attempt connection
  {
    Serial.println(F("Connection to caster failed"));
    return (false);
  }
  else
  {
    Serial.print(F("Connected to "));
    Serial.print(casterHost);
    Serial.print(F(" : "));
    Serial.println(casterPort);

    Serial.print(F("Requesting NTRIP Data from mount point "));
    Serial.println(mountPoint);

    // Set up the server request (GET)
    const int SERVER_BUFFER_SIZE = 512;
    char serverRequest[SERVER_BUFFER_SIZE];
    snprintf(serverRequest,
             SERVER_BUFFER_SIZE,
             "GET /%s HTTP/1.0\r\nUser-Agent: NTRIP SparkFun u-blox Client v1.0\r\n",
             mountPoint);

    // Set up the credentials
    char credentials[512];
    if (strlen(casterUser) == 0)
    {
      strncpy(credentials, "Accept: */*\r\nConnection: close\r\n", sizeof(credentials));
    }
    else
    {
      //Pass base64 encoded user:pw
      char userCredentials[sizeof(casterUser) + sizeof(casterUserPW) + 1]; //The ':' takes up a spot
      snprintf(userCredentials, sizeof(userCredentials), "%s:%s", casterUser, casterUserPW);

      Serial.print(F("Sending credentials: "));
      Serial.println(userCredentials);

#if defined(ARDUINO_ARCH_ESP32)
      //Encode with ESP32 built-in library
      base64 b;
      String strEncodedCredentials = b.encode(userCredentials);
      char encodedCredentials[strEncodedCredentials.length() + 1];
      strEncodedCredentials.toCharArray(encodedCredentials, sizeof(encodedCredentials)); //Convert String to char array
#else
      //Encode with nfriendly library
      int encodedLen = base64_enc_len(strlen(userCredentials));
      char encodedCredentials[encodedLen];                                         //Create array large enough to house encoded data
      base64_encode(encodedCredentials, userCredentials, strlen(userCredentials)); //Note: Input array is consumed
#endif

      snprintf(credentials, sizeof(credentials), "Authorization: Basic %s\r\n", encodedCredentials);
    }

    // Add the encoded credentials to the server request
    strncat(serverRequest, credentials, SERVER_BUFFER_SIZE);
    strncat(serverRequest, "\r\n", SERVER_BUFFER_SIZE);

    Serial.print(F("serverRequest size: "));
    Serial.print(strlen(serverRequest));
    Serial.print(F(" of "));
    Serial.print(sizeof(serverRequest));
    Serial.println(F(" bytes available"));

    // Send the server request
    Serial.println(F("Sending server request: "));
    Serial.println(serverRequest);
    ntripClient.write((const uint8_t*)serverRequest, strlen(serverRequest));

    //Wait up to 5 seconds for response
    unsigned long startTime = millis();
    while (ntripClient.available() == 0)
    {
      if (millis() > (startTime + 5000))
      {
        Serial.println(F("Caster timed out!"));
        ntripClient.stop();
        return (false);
      }
      delay(10);
    }

    //Check reply
    int connectionResult = 0;
    char response[512];
    size_t responseSpot = 0;
    while (ntripClient.available()) // Read bytes from the caster and store them
    {
      if (responseSpot == sizeof(response) - 1) // Exit the loop if we get too much data
        break;

      response[responseSpot++] = ntripClient.read();

      if (connectionResult == 0) // Only print success/fail once
      {
        if (strstr(response, "200") != NULL) //Look for '200 OK'
        {
          connectionResult = 200;
        }
        if (strstr(response, "401") != NULL) //Look for '401 Unauthorized'
        {
          Serial.println(F("Hey - your credentials look bad! Check your caster username and password."));
          connectionResult = 401;
        }
      }
    }
    response[responseSpot] = '\0'; // NULL-terminate the response

    //Serial.print(F("Caster responded with: ")); Serial.println(response); // Uncomment this line to see the full response

    if (connectionResult != 200)
    {
      Serial.print(F("Failed to connect to "));
      Serial.println(casterHost);
      return (false);
    }
    else
    {
      Serial.print(F("Connected to: "));
      Serial.println(casterHost);
      lastReceivedRTCM_ms = millis(); //Reset timeout
    }
  } //End attempt to connect

  return (true);
} // /beginClient

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Check for the arrival of any correction data. Push it to the GNSS.
//Return false if: the connection has dropped, or if we receive no data for maxTimeBeforeHangup_ms
bool processConnection()
{
  if (ntripClient.connected() == true) // Check that the connection is still open
  {
    // uint8_t rtcmData[512 * 4]; //Most incoming data is around 500 bytes but may be larger
    uint8_t rtcmData[512 * 4]; //Most incoming data is around 500 bytes but may be larger
    size_t rtcmCount = 0;

    //Collect any available RTCM data
    while (ntripClient.available())
    {
      //Serial.write(ntripClient.read()); //Pipe to serial port is fine but beware, it's a lot of binary data!
      rtcmData[rtcmCount++] = ntripClient.read();
      if (rtcmCount == sizeof(rtcmData))
        break;
    }

    if (rtcmCount > 0)
    {
      lastReceivedRTCM_ms = millis();

      //Push RTCM to GNSS module over I2C
      myGNSS.pushRawData(rtcmData, rtcmCount);

      Serial.print(F("Pushed "));
      Serial.print(rtcmCount);
      Serial.println(F(" RTCM bytes to ZED"));
      //DEBUG IP
      // IPAddress local = modem.localIP();
      // Serial.print("Local IP: ");
      // Serial.println(local);

    }
  }
  else
  {
    Serial.println(F("Connection dropped!"));
    return (false); // Connection has dropped - return false
  }

  //Timeout if we don't have new data for maxTimeBeforeHangup_ms
  if ((millis() - lastReceivedRTCM_ms) > maxTimeBeforeHangup_ms)
  {
    Serial.println(F("RTCM timeout!"));
    return (false); // Connection has timed out - return false
  }

  return (true);
} // /processConnection

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void closeConnection()
{
  if (ntripClient.connected() == true)
  {
    ntripClient.stop();
  }
  Serial.println(F("Disconnected!"));
  ESP.restart(); //TODO:resolve, delay time, bug infinity reconnect ntrip if base RTK down
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Return true if a key has been pressed
bool keyPressed()
{
  if (Serial.available()) // Check for a new key press
  {
    delay(100); // Wait for any more keystrokes to arrive
    while (Serial.available()) // Empty the serial buffer
      Serial.read();
    return (true);
  }

  return (false);
}

// MODEM ON from deepsleep
void modem_on()
{
  // Set-up modem  power pin
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  delay(10);
  digitalWrite(POWER_PIN, LOW);
  delay(1010); //Ton 1sec
  digitalWrite(POWER_PIN, HIGH);

  //wait_till_ready();
  Serial.println("Waiting till modem ready...");
  delay(4510); //Ton uart 4.5sec but seems to need ~7sec after hard (button) reset
               //On soft-reset serial replies immediately.
}

// MODEM OFF for DeepSleep
void modem_off()
{
  //if you turn modem off while activating the fancy sleep modes it takes ~20sec, else its immediate
  Serial.println("Going to sleep now with modem turned off");
  //modem.gprsDisconnect();
  //modem.radioOff();
  modem.sleepEnable(false); // required in case sleep was activated and will apply after reboot
  modem.poweroff();
}