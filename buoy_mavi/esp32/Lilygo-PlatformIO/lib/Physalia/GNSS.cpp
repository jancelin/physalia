#include "GNSS.h"
#include <Wire.h>

GNSS::GNSS(){
    this->pin_GNSS = 32;
    this->GNSS_FREQ = 1;
    // this->myGNSS;

}
GNSS::~GNSS(){}

void GNSS::setup(){
    Serial.println(F("GNSS SETUP"));

    pinMode(this->pin_GNSS,OUTPUT);
    digitalWrite(this->pin_GNSS, LOW);

    Wire.begin(); //Start I2C
    while (this->myGNSS.begin() == false) //Connect to the Ublox module using Wire port
    {
        Serial.println(F("u-blox GPS not detected at default I2C address. Please check wiring."));
        delay(2000);
    }
    Serial.println(F("u-blox module connected"));

    this->myGNSS.setI2COutput(COM_TYPE_UBX | COM_TYPE_NMEA);                                //Set the I2C port to output both NMEA and UBX messages
    this->myGNSS.setPortInput(COM_PORT_I2C, COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3); //Be sure RTCM3 input is enabled. UBX + RTCM3 is not a valid state.
    this->myGNSS.setDGNSSConfiguration(SFE_UBLOX_DGNSS_MODE_FIXED); // Set the differential mode - ambiguities are fixed whenever possible
    this->myGNSS.setNavigationFrequency(this->GNSS_FREQ); //Set output in Hz.

    // Set the Main Talker ID to "GP". The NMEA GGA messages will be GPGGA instead of GNGGA
    this->myGNSS.setMainTalkerID(SFE_UBLOX_MAIN_TALKER_ID_GP);
    this->myGNSS.setNMEAGPGGAcallbackPtr(&GNSS::physalia_pushGPGGA ); // Set up the callback for GPGGA
    this->myGNSS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_I2C, 60); // Tell the module to output GGA every 60 seconds
    // this->myGNSS.setAutoPVTcallbackPtr(&GNSS::printPVTdata); // Enable automatic NAV PVT messages with callback to printPVTdata so we can watch the carrier solution go to fixed
    //myGNSS.saveConfiguration(VAL_CFG_SUBSEC_IOPORT | VAL_CFG_SUBSEC_MSGCONF); //Optional: Save the ioPort and message settings to NVM

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
}

void GNSS::process(){

}

//GNSS=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Callback: physalia_pushGPGGA will be called when new GPGGA NMEA data arrives
// See u-blox_structs.h for the full definition of NMEA_GGA_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setNMEAGPGGAcallback
//        /               _____  This _must_ be NMEA_GGA_data_t
//        |              /           _____ You can use any name you like for the struct
//        |              |          /
//        |              |          |
void GNSS::physalia_pushGPGGA(NMEA_GGA_data_t *nmeaData)
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


// Callback: printPVTdata will be called when new NAV PVT data arrives
// See u-blox_structs.h for the full definition of UBX_NAV_PVT_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setAutoPVTcallback
//        /                  _____  This _must_ be UBX_NAV_PVT_data_t
//        |                 /               _____ You can use any name you like for the struct
//        |                 |              /
//        |                 |              |
// static void printPVTdata(UBX_NAV_PVT_data_t *ubxDataStruct)
// {
//   long now = millis();
//   // allocate the memory for the document
//   StaticJsonDocument<256> doc;
//   // create an object
//   JsonObject object = doc.to<JsonObject>();

//   //doc["capteur"] = matUuid + WiFi.macAddress()+"'";//matUuid; // Print capteur uuid
//   doc["capteur"] = matUuid; // Print capteur uuid

//   uint16_t y = ubxDataStruct->year; // Print the year
//   uint8_t mo = ubxDataStruct->month; // Print the year
//   String mo1;
//   if (mo < 10) {mo1 = "0"+ String(mo);} else { mo1 = String(mo);};
//   uint8_t d = ubxDataStruct->day; // Print the year
//   String d1;
//     if (d < 10) {d1 = "0"+ String(d);} else { d1 = String(d);};
//   uint8_t h = ubxDataStruct->hour; // Print the hours
//   String h1;
//     if (h < 10) {h1 = "0"+ String(h);} else { h1 = String(h);};
//   uint8_t m = ubxDataStruct->min; // Print the minutes
//   String m1;
//     if (m < 10) {m1 = "0"+ String(m);} else { m1 = String(m);};
//   uint8_t s = ubxDataStruct->sec; // Print the seconds
//   String s1;
//     if (s < 10) {s1 = "0"+ String(s);} else { s1 = String(s);};
//   unsigned long millisecs = ubxDataStruct->iTOW % 1000; // Print the milliseconds
//   String a = ":";
//   String b = "-";
//   String date1 = y+b+mo1+b+d1+" ";
//   String time1 = h1 +a+ m1 +a+ s1 + "." + millisecs;
//   object["datetime"] = "'"+date1+time1+"'"; // print date time for postgresql data injection.

//   double latitude = ubxDataStruct->lat; // Print the latitude
//   doc["lat"] = latitude / 10000000.0;

//   double longitude = ubxDataStruct->lon; // Print the longitude
//   doc["lon"] = longitude / 10000000.0;

//   double elevation = ubxDataStruct->height; // Print the height above mean sea level
//   doc["elv_m"] = elevation / 1000.0;

//   double altitude = ubxDataStruct->hMSL; // Print the height above mean sea level
//   doc["alt_m"] = altitude / 1000.0;

//   uint8_t fixType = ubxDataStruct->fixType; // Print the fix type
//   // 0 none/1 Dead reckoning/2 2d/3 3d/4 GNSS + Dead reckoning/ 5 time only
//   doc["fix"] = fixType;

//   uint8_t carrSoln = ubxDataStruct->flags.bits.carrSoln; // Print the carrier solution
//   // 0 none/1 floating/ 2 Fixed
//   doc["car"] = carrSoln;

//   uint32_t hAcc = ubxDataStruct->hAcc; // Print the horizontal accuracy estimate
//   doc["hacc_mm"] = hAcc;

//   uint32_t vAcc = ubxDataStruct->vAcc; // Print the vertical accuracy estimate
//   doc["vacc_mm"] = vAcc;

//   uint8_t numSV = ubxDataStruct->numSV; // Print tle number of SVs used in nav solution
//   doc["numsv"] = numSV;

//   serializeJson(doc, Serial);
//   Serial.println();
// }