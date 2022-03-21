/*
  Use ESP32 WiFi to get RTCM data from Swift Navigation's Skylark caster as a Client, and transmit GGA using a callback
  By: SparkFun Electronics / Nathan Seidle & Paul Clark
  Date: January 13th, 2022
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.
  This example shows how to obtain RTCM data from a NTRIP Caster over WiFi and push it over I2C to a ZED-F9x.
  It's confusing, but the Arduino is acting as a 'client' to a 'caster'.
  In this case we will use Skylark. But you can of course use RTK2Go or Emlid's Caster too. Change secrets.h. as required.
  The rover's location will be broadcast to the caster every 10s via GGA setence - automatically using a callback.
  This is a proof of concept to show how to connect to a caster via HTTP and show how the corrections control the accuracy.
  It's a fun thing to disconnect from the caster and watch the accuracy degrade. Then connect again and watch it recover!
  For more information about NTRIP Clients and the differences between Rev1 and Rev2 of the protocol
  please see: https://www.use-snip.com/kb/knowledge-base/ntrip-rev1-versus-rev2-formats/
  "In broad protocol terms, the NTRIP client must first connect (get an HTTP “OK” reply) and only then
  should it send the sentence.  NTRIP protocol revision 2 (which does not have very broad industry
  acceptance at this time) does allow sending the sentence in the original header."
  https://www.use-snip.com/kb/knowledge-base/subtle-issues-with-using-ntrip-client-nmea-183-strings/
  Feel like supporting open source hardware?
  Buy a board from SparkFun!
  ZED-F9P RTK2: https://www.sparkfun.com/products/16481
  RTK Surveyor: https://www.sparkfun.com/products/18443
  RTK Express: https://www.sparkfun.com/products/18442
  Hardware Connections:
  Plug a Qwiic cable into the GNSS and a ESP32 Thing Plus
  If you don't have a platform with a Qwiic connection use the SparkFun Qwiic Breadboard Jumper (https://www.sparkfun.com/products/14425)
  Open the serial monitor at 115200 baud to see the output
*/

/*
ESP32 pin: https://drive.google.com/file/d/1do7tb6tHbTQNlPX8fVwQQnwxOe8gZbed/view
Drotek dp0601 Pin description: https://raw.githubusercontent.com/drotek/datasheets/master/DrotekDoc_0891B08A%20-%20DP0601%20GNSS%20RTK%20(F9P).pdf
ESP32 I2C communication: https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
Install Libraries arduino: https://knowledge.parcours-performance.com/librairies-arduino-installer/
Sparfun_u-blox_gnns_arduino_library: https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library
JsonDocument capacity: https://arduinojson.org/v6/assistant/
NAV-PVT :http://docs.ros.org/en/noetic/api/ublox_msgs/html/msg/NavPVT.html
mqtt client: https://techtutorialsx.com/2017/04/24/esp32-publishing-messages-to-mqtt-topic/
auto reconnect wifi: http://community.heltec.cn/t/solved-wifi-reconnect/1396/3
*/

#include <WiFi.h>
#include "secrets.h"

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;

#include <ArduinoJson.h>
#include <PubSubClient.h>

#include <Wire.h>
 
WiFiClient espClient;

//The ESP32 core has a built in base64 library but not every platform does
//We'll use an external lib if necessary.
#if defined(ARDUINO_ARCH_ESP32)
#include "base64.h" //Built-in ESP32 library
#else
#include <Base64.h> //nfriendly library from https://github.com/adamvr/arduino-base64, will work with any platform
#endif

// TTGO T-Call pins
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22
// GNSS pins
#define I2C_SDA_2            18
#define I2C_SCL_2            19

TwoWire I2CBME = TwoWire(1);

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
PubSubClient client(espClient); //MQTT
long lastReconnectAttempt = 0; 

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

boolean reconnect() {
  if (client.connect(mqtttopic)) {
    // Once connected, publish an announcement...
    client.publish(mqtttopic, matUuid);
    // ... and resubscribeI2CBME
    client.subscribe(mqtttopic);
  }
  return client.connected();
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//Global variables

unsigned long lastReceivedRTCM_ms = 0;          //5 RTCM messages take approximately ~300ms to arrive at 115200bps
const unsigned long maxTimeBeforeHangup_ms = 10000UL; //If we fail to get a complete RTCM frame after 10s, then disconnect from caster

//bool transmitLocation = true;  change to secrets.h      //By default we will transmit the unit's location via GGA sentence.

WiFiClient ntripClient; // The WiFi connection to the NTRIP server. This is global so pushGGA can see if we are connected.

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

  // allocate the memory for the document
  StaticJsonDocument<256> doc;
  // create an object
  JsonObject object = doc.to<JsonObject>();

  doc["capteur"] = matUuid + WiFi.macAddress()+"'";//matUuid; // Print capteur uuid

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

  uint32_t vAcc = ubxDataStruct->vAcc; // Print the horizontal accuracy estimate
  doc["vacc_mm"] = vAcc;

  uint8_t numSV = ubxDataStruct->numSV; // Print tle number of SVs used in nav solution
  doc["numsv"] = numSV;

  serializeJson(doc, Serial);
  Serial.println();

  //Send position to MQTT broker
  String msg;
  String output = "JSON = ";
  serializeJson(doc, msg);
  client.publish(mqtttopic, msg.c_str());
  Serial.println("Message send");
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void setup()
{
  Serial.begin(115200);
  
  Serial.println(F("NTRIP testing"));

  Wire.begin(I2C_SDA_2, I2C_SCL_2); //Start I2C
  
  while (myGNSS.begin() == false) //Connect to the Ublox module using Wire port
  {
    Serial.println(F("u-blox GPS not detected at default I2C address. Please check wiring."));
    delay(2000);
  }
  Serial.println(F("u-blox module connected"));

  myGNSS.setI2COutput(COM_TYPE_UBX | COM_TYPE_NMEA);                                //Set the I2C port to output both NMEA and UBX messages
  myGNSS.setPortInput(COM_PORT_I2C, COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3); //Be sure RTCM3 input is enabled. UBX + RTCM3 is not a valid state.

  myGNSS.setDGNSSConfiguration(SFE_UBLOX_DGNSS_MODE_FIXED); // Set the differential mode - ambiguities are fixed whenever possible

  myGNSS.setNavigationFrequency(1); //Set output in Hz.

  // Set the Main Talker ID to "GP". The NMEA GGA messages will be GPGGA instead of GNGGA
  myGNSS.setMainTalkerID(SFE_UBLOX_MAIN_TALKER_ID_GP);

  myGNSS.setNMEAGPGGAcallbackPtr(&pushGPGGA); // Set up the callback for GPGGA

  myGNSS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_I2C, 10); // Tell the module to output GGA every 10 seconds

  myGNSS.setAutoPVTcallbackPtr(&printPVTdata); // Enable automatic NAV PVT messages with callback to printPVTdata so we can watch the carrier solution go to fixed

  //myGNSS.saveConfiguration(VAL_CFG_SUBSEC_IOPORT | VAL_CFG_SUBSEC_MSGCONF); //Optional: Save the ioPort and message settings to NVM

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  bool keepTrying = true;
  while (keepTrying)
  {
    Serial.print(F("Connecting to local WiFi"));

    unsigned long startTime = millis();
    WiFi.begin(ssid, password);
    while ((WiFi.status() != WL_CONNECTED) && (millis() < (startTime + 10000))) // Timeout after 10 seconds
    {
      delay(500);
      Serial.print(F("."));
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
      keepTrying = false; // Connected!
    else
    {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    }
  }

  Serial.println(F("WiFi connected with IP: "));
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  delay(500); 

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
 
      Serial.println("connected");
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
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
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected

    client.loop();
  }
  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Wifi auto reconnect
  delay(1000);
  if(WiFi.status() == WL_CONNECTED){
    //Serial.println("WIFI connect!!!!");
    }
  else{
    Serial.println("WIFI disconnected. reconnect...");
    WiFi.reconnect();
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
    ntripClient.write(serverRequest, strlen(serverRequest));

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
