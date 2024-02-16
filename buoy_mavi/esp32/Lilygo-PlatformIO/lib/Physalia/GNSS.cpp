#include "GNSS.h"
#include <Wire.h>
#include "DataFormat.h"

#define transmitLocation false

Modem GNSS::modem;
StaticJsonDocument<256> GNSS::doc;

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
    this->myGNSS.setI2CInput(COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3); //Be sure RTCM3 input is enabled. UBX + RTCM3 is not a valid state.
    this->myGNSS.setDGNSSConfiguration(SFE_UBLOX_DGNSS_MODE_FIXED); // Set the differential mode - ambiguities are fixed whenever possible
    this->myGNSS.setNavigationFrequency(this->GNSS_FREQ); //Set output in Hz.

    // Set the Main Talker ID to "GP". The NMEA GGA messages will be GPGGA instead of GNGGA
    this->myGNSS.setMainTalkerID(SFE_UBLOX_MAIN_TALKER_ID_GP);
    this->myGNSS.setNMEAGPGGAcallbackPtr(&GNSS::physalia_pushGPGGA ); // Set up the callback for GPGGA
    this->myGNSS.setVal8(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_I2C, 60); // Tell the module to output GGA every 60 seconds
    this->myGNSS.setAutoPVTcallbackPtr(&GNSS::physalia_printPVTdata); // Enable automatic NAV PVT messages with callback to printPVTdata so we can watch the carrier solution go to fixed
    //myGNSS.saveConfiguration(VAL_CFG_SUBSEC_IOPORT | VAL_CFG_SUBSEC_MSGCONF); //Optional: Save the ioPort and message settings to NVM

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
}

/* Called every Loop */
void GNSS::process(){
  this->myGNSS.checkUblox();
  this->myGNSS.checkCallbacks();
}

StaticJsonDocument<256> GNSS::getDocJson() {
    return GNSS::doc;
};

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
  if ((GNSS::modem.connected() == true) && (transmitLocation == true))
  {
    Serial.print(F("Pushing GGA to server: "));
    Serial.print((const char *)nmeaData->nmea); // .nmea is printable (NULL-terminated) and already has \r\n on the end

    //Push our current GGA sentence to caster
    GNSS::modem.print(reinterpret_cast<const char*>(nmeaData->nmea));
  }
}


// Callback: physalia_printPVTdata will be called when new NAV PVT data arrives
// See u-blox_structs.h for the full definition of UBX_NAV_PVT_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setAutoPVTcallback
//        /                  _____  This _must_ be UBX_NAV_PVT_data_t
//        |                 /               _____ You can use any name you like for the struct
//        |                 |              /
//        |                 |              |
void GNSS::physalia_printPVTdata(UBX_NAV_PVT_data_t *ubxDataStruct)
{
  DataFormat data(ubxDataStruct);
  doc = data.getDocJson();

  serializeJson(doc, Serial);
  Serial.println();
}