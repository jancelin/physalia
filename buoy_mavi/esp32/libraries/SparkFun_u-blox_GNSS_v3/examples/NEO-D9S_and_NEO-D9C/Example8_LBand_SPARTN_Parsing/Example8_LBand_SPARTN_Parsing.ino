/*
  Use the NEO-D9S L-Band receiver to provide corrections to a ZED-F9x as SPARTN data
  By: SparkFun Electronics / Paul Clark
  Based on original code by: u-blox AG / Michael Ammann
  v3 updates: Decembe 22nd, 2022
  License: MIT. See license file for more information.

  This example shows how to obtain SPARTN correction data from a NEO-D9S L-Band receiver and push it over ESP32 Serial (UART) to a ZED-F9x.
  This example parses (extracts) SPARTN packets from the L-Band data stream and validates them before pushing.

  This is a proof of concept to show how the UBX-RXM-PMP corrections control the accuracy.

  You will need a Thingstream PointPerfect account to be able to access the SPARTN Credentials (L-Band or L-Band + IP Dynamic Keys).
  Copy and paste the Current Key and Next Key into secrets.h.

  Feel like supporting open source hardware?
  Buy a board from SparkFun!
  ZED-F9P RTK2: https://www.sparkfun.com/products/16481
  NEO-D9S:      https://www.sparkfun.com/products/19390
  Combo Board:  https://www.sparkfun.com/products/20167

  Hardware Connections:
  Use Qwiic cables to connect the NEO-D9S and ZED-F9x GNSS to your board
  If you don't have a platform with a Qwiic connection use the SparkFun Qwiic Breadboard Jumper (https://www.sparkfun.com/products/14425)
  Open the serial monitor at 115200 baud to see the output
*/

#include "secrets.h" // <- Copy and paste the Current Key and Next Key into secrets.h

#include <driver/uart.h>      //Required for uart_set_rx_full_threshold() on cores <v2.0.5
HardwareSerial serialGNSS(2); // ESP32 UART2: TX on 17, RX on 16

#include <SparkFun_u-blox_GNSS_v3.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v3
SFE_UBLOX_GNSS myGNSS; // ZED-F9x
SFE_UBLOX_GNSS myLBand; // NEO-D9S

const uint32_t myLBandFreq = 1556290000; // Uncomment this line to use the US SPARTN 1.8 service
//const uint32_t myLBandFreq = 1545260000; // Uncomment this line to use the EU SPARTN 1.8 service

#define OK(ok) (ok ? F("  ->  OK") : F("  ->  ERROR!")) // Convert uint8_t into OK/ERROR

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Callback: pushRXMPMP will be called when new PMP data arrives
// See u-blox_structs.h for the full definition of UBX_RXM_PMP_message_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setRXMPMPmessageCallbackPtr
//        /               _____  This _must_ be UBX_RXM_PMP_message_data_t
//        |              /              _____ You can use any name you like for the struct
//        |              |             /
//        |              |             |
void pushRXMPMP(UBX_RXM_PMP_message_data_t *pmpData)
{
  //Extract the raw message payload length
  uint16_t payloadLen = ((uint16_t)pmpData->lengthMSB << 8) | (uint16_t)pmpData->lengthLSB;

  uint16_t numBytesUserData = pmpData->payload[2] | ((uint16_t)pmpData->payload[3] << 8);
  uint16_t fecBits = pmpData->payload[20] | ((uint16_t)pmpData->payload[21] << 8);
  float ebno = (float)pmpData->payload[22] / 8;
  
  Serial.print(F("New RXM-PMP data received. userData: "));
  Serial.print(numBytesUserData);
  Serial.print(F(" Bytes. fecBits: "));
  Serial.print(fecBits);
  Serial.print(F(". ebno (dB): "));
  Serial.print(ebno);
  Serial.println(F("."));

  //Parse the SPARTN data stream contained in the userData
  for (uint16_t i = 0; i < numBytesUserData; i++)
  {
    bool valid = false;
    uint16_t len;
    uint8_t *spartn = myLBand.parseSPARTN(pmpData->payload[24 + i], valid, len);

    if (valid)
    {
      Serial.print(F("Valid SPARTN data parsed. Pushing "));
      Serial.print(len);
      Serial.println(F(" bytes to the ZED-F9P..."));
      
      //myGNSS.pushRawData(spartn, len); // Push the SPARTN data to the module using I2C

      serialGNSS.write(spartn, len); // Push the SPARTN data to the module using Serial
    }
  }

  //myGNSS.pushRawData(&pmpData->sync1, (size_t)payloadLen + 6); // Push the sync chars, class, ID, length and payload
  //myGNSS.pushRawData(&pmpData->checksumA, (size_t)2); // Push the checksum bytes

  //serialGNSS.write(&pmpData->sync1, (size_t)payloadLen + 6); // Push the sync chars, class, ID, length and payload
  //serialGNSS.write(&pmpData->checksumA, (size_t)2); // Push the checksum bytes

}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Callback: printPVTdata will be called when new NAV PVT data arrives
// See u-blox_structs.h for the full definition of UBX_NAV_PVT_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setAutoPVTcallbackPtr
//        /                  _____  This _must_ be UBX_NAV_PVT_data_t
//        |                 /               _____ You can use any name you like for the struct
//        |                 |              /
//        |                 |              |
void printPVTdata(UBX_NAV_PVT_data_t *ubxDataStruct)
{
  double latitude = ubxDataStruct->lat; // Print the latitude
  Serial.print(F("Lat: "));
  Serial.print(latitude / 10000000.0, 7);

  double longitude = ubxDataStruct->lon; // Print the longitude
  Serial.print(F("  Long: "));
  Serial.print(longitude / 10000000.0, 7);

  double altitude = ubxDataStruct->hMSL; // Print the height above mean sea level
  Serial.print(F("  Height: "));
  Serial.print(altitude / 1000.0, 3);

  uint8_t fixType = ubxDataStruct->fixType; // Print the fix type
  Serial.print(F("  Fix: "));
  Serial.print(fixType);
  if (fixType == 0)
    Serial.print(F(" (None)"));
  else if (fixType == 1)
    Serial.print(F(" (Dead Reckoning)"));
  else if (fixType == 2)
    Serial.print(F(" (2D)"));
  else if (fixType == 3)
    Serial.print(F(" (3D)"));
  else if (fixType == 4)
    Serial.print(F(" (GNSS + Dead Reckoning)"));
  else if (fixType == 5)
    Serial.print(F(" (Time Only)"));
  else
    Serial.print(F(" (UNKNOWN)"));

  uint8_t carrSoln = ubxDataStruct->flags.bits.carrSoln; // Print the carrier solution
  Serial.print(F("  Carrier Solution: "));
  Serial.print(carrSoln);
  if (carrSoln == 0)
    Serial.print(F(" (None)"));
  else if (carrSoln == 1)
    Serial.print(F(" (Floating)"));
  else if (carrSoln == 2)
    Serial.print(F(" (Fixed)"));
  else
    Serial.print(F(" (UNKNOWN)"));

  uint32_t hAcc = ubxDataStruct->hAcc; // Print the horizontal accuracy estimate
  Serial.print(F("  Horizontal Accuracy Estimate: "));
  Serial.print(hAcc);
  Serial.print(F(" (mm)"));

  Serial.println();    
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Callback: printRXMCOR will be called when new RXM COR data arrives
// See u-blox_structs.h for the full definition of UBX_RXM_COR_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setRXMCORcallbackPtr
//        /                  _____  This _must_ be UBX_RXM_COR_data_t
//        |                 /               _____ You can use any name you like for the struct
//        |                 |              /
//        |                 |              |
void printRXMCOR(UBX_RXM_COR_data_t *ubxDataStruct)
{
  Serial.print(F("UBX-RXM-COR:  ebno: "));
  Serial.print((double)ubxDataStruct->ebno / 8, 3); //Convert ebno to dB

  Serial.print(F("  protocol: "));
  if (ubxDataStruct->statusInfo.bits.protocol == 1)
    Serial.print(F("RTCM3"));
  else if (ubxDataStruct->statusInfo.bits.protocol == 2)
    Serial.print(F("SPARTN"));
  else if (ubxDataStruct->statusInfo.bits.protocol == 29)
    Serial.print(F("PMP (SPARTN)"));
  else if (ubxDataStruct->statusInfo.bits.protocol == 30)
    Serial.print(F("QZSSL6"));
  else
    Serial.print(F("Unknown"));

  Serial.print(F("  errStatus: "));
  if (ubxDataStruct->statusInfo.bits.errStatus == 1)
    Serial.print(F("Error-free"));
  else if (ubxDataStruct->statusInfo.bits.errStatus == 2)
    Serial.print(F("Erroneous"));
  else
    Serial.print(F("Unknown"));

  Serial.print(F("  msgUsed: "));
  if (ubxDataStruct->statusInfo.bits.msgUsed == 1)
    Serial.print(F("Not used"));
  else if (ubxDataStruct->statusInfo.bits.msgUsed == 2)
    Serial.print(F("Used"));
  else
    Serial.print(F("Unknown"));

  Serial.print(F("  msgEncrypted: "));
  if (ubxDataStruct->statusInfo.bits.msgEncrypted == 1)
    Serial.print(F("Not encrypted"));
  else if (ubxDataStruct->statusInfo.bits.msgEncrypted == 2)
    Serial.print(F("Encrypted"));
  else
    Serial.print(F("Unknown"));

  Serial.print(F("  msgDecrypted: "));
  if (ubxDataStruct->statusInfo.bits.msgDecrypted == 1)
    Serial.print(F("Not decrypted"));
  else if (ubxDataStruct->statusInfo.bits.msgDecrypted == 2)
    Serial.print(F("Successfully decrypted"));
  else
    Serial.print(F("Unknown"));

  Serial.println();
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void setup()
{
  Serial.begin(115200);
  Serial.println(F("NEO-D9S SPARTN Corrections"));

  serialGNSS.begin(38400); // UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to use the same rate.
                                           
  Wire.begin(); //Start I2C

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Begin and configure the ZED-F9x

  //myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  while (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS module not detected at default I2C address. Please check wiring."));
    delay(2000);
  }
  Serial.println(F("u-blox GNSS module connected"));

  uint8_t ok = myGNSS.setI2COutput(COM_TYPE_UBX); //Turn off NMEA noise
  if (ok) ok = myGNSS.setI2CInput(COM_TYPE_UBX);
  if (ok) ok = myGNSS.setUART1Output(0);
  if (ok) ok = myGNSS.setUART1Input(0);
  if (ok) ok = myGNSS.setUART2Output(0);
  if (ok) ok = myGNSS.setUART2Input(COM_TYPE_SPARTN); //Be sure SPARTN input is enabled
  if (ok) ok = myGNSS.setDGNSSConfiguration(SFE_UBLOX_DGNSS_MODE_FIXED); // Set the differential mode - ambiguities are fixed whenever possible
  if (ok) ok = myGNSS.setNavigationFrequency(1); //Set output in Hz.
  if (ok) ok = myGNSS.setVal8(UBLOX_CFG_SPARTN_USE_SOURCE, 0); // Use "IP" source, not L-Band. We are injecting raw SPARTN, not PMP
  if (ok) ok = myGNSS.setVal8(UBLOX_CFG_MSGOUT_UBX_RXM_COR_I2C, 1); // Enable UBX-RXM-COR messages on I2C
  
  //Configure the SPARTN IP Dynamic Keys
  //"When the receiver boots, the host should send 'current' and 'next' keys in one message." - Use setDynamicSPARTNKeys for this.
  //"Every time the 'current' key is expired, 'next' takes its place."
  //"Therefore the host should then retrieve the new 'next' key and send only that." - Use setDynamicSPARTNKey for this.
  // The key can be provided in binary (uint8_t) format or in ASCII Hex (char) format, but in both cases keyLengthBytes _must_ represent the binary key length in bytes.
  if (ok) ok = myGNSS.setDynamicSPARTNKeys(currentKeyLengthBytes, currentKeyGPSWeek, currentKeyGPSToW, currentDynamicKey,
                                           nextKeyLengthBytes, nextKeyGPSWeek, nextKeyGPSToW, nextDynamicKey);

  //if (ok) ok = myGNSS.saveConfiguration(VAL_CFG_SUBSEC_IOPORT | VAL_CFG_SUBSEC_MSGCONF); //Optional: Save the ioPort and message settings to NVM and BBR
  
  Serial.print(F("GNSS: configuration "));
  Serial.println(OK(ok));

  myGNSS.setAutoPVTcallbackPtr(&printPVTdata); // Enable automatic NAV PVT messages with callback to printPVTdata so we can watch the carrier solution go to fixed

  myGNSS.setRXMCORcallbackPtr(&printRXMCOR); // Print the contents of UBX-RXM-COR messages so we can check if the PMP data is being decrypted successfully

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Begin and configure the NEO-D9S L-Band receiver

  //myLBand.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  while (myLBand.begin(Wire, 0x43) == false) //Connect to the u-blox NEO-D9S using Wire port. The D9S default I2C address is 0x43 (not 0x42)
  {
    Serial.println(F("u-blox NEO-D9S not detected at default I2C address. Please check wiring."));
    delay(2000);
  }
  Serial.println(F("u-blox NEO-D9S connected"));

  myLBand.newCfgValset(); // Create a new Configuration Interface message - this defaults to VAL_LAYER_RAM_BBR (change in RAM and BBR)
  myLBand.addCfgValset(UBLOX_CFG_PMP_CENTER_FREQUENCY,     myLBandFreq); // Default 1539812500 Hz
  myLBand.addCfgValset(UBLOX_CFG_PMP_SEARCH_WINDOW,        2200);        // Default 2200 Hz
  myLBand.addCfgValset(UBLOX_CFG_PMP_USE_SERVICE_ID,       0);           // Default 1 
  myLBand.addCfgValset(UBLOX_CFG_PMP_SERVICE_ID,           21845);       // Default 50821
  myLBand.addCfgValset(UBLOX_CFG_PMP_DATA_RATE,            2400);        // Default 2400 bps
  myLBand.addCfgValset(UBLOX_CFG_PMP_USE_DESCRAMBLER,      1);           // Default 1
  myLBand.addCfgValset(UBLOX_CFG_PMP_DESCRAMBLER_INIT,     26969);       // Default 23560
  myLBand.addCfgValset(UBLOX_CFG_PMP_USE_PRESCRAMBLING,    0);           // Default 0
  myLBand.addCfgValset(UBLOX_CFG_PMP_UNIQUE_WORD,          16238547128276412563ull); 
  myLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_I2C,   1);           // Ensure UBX-RXM-PMP is enabled on the I2C port 
  myLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_UART1, 1);           // Output UBX-RXM-PMP on UART1
  myLBand.addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX,         1);           // Enable UBX output on UART2
  myLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_UART2, 1);           // Output UBX-RXM-PMP on UART2
  myLBand.addCfgValset(UBLOX_CFG_UART1_BAUDRATE,           38400);       // match baudrate with ZED default
  myLBand.addCfgValset(UBLOX_CFG_UART2_BAUDRATE,           38400);       // match baudrate with ZED default
  ok = myLBand.sendCfgValset(); // Apply the settings
  
  Serial.print(F("L-Band: configuration "));
  Serial.println(OK(ok));

  myLBand.softwareResetGNSSOnly(); // Do a restart

  myLBand.setRXMPMPmessageCallbackPtr(&pushRXMPMP); // Call pushRXMPMP when new PMP data arrives. Push it to the GNSS

}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new GNSS data and process it.
  myGNSS.checkCallbacks(); // Check if any GNSS callbacks are waiting to be processed.

  myLBand.checkUblox(); // Check for the arrival of new PMP data and process it.
  myLBand.checkCallbacks(); // Check if any LBand callbacks are waiting to be processed.
}
