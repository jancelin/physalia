#ifndef GNSS_H
#define GNSS_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
#include <Wire.h>
#include "config.h"

class GNSS{
    public:
        GNSS();
        ~GNSS();
        void setup();
        void process();
        static void physalia_pushGPGGA(NMEA_GGA_data_t *nmeaData);
        //void printPVTdata(UBX_NAV_PVT_data_t *ubxDataStruct);

    private:
        int pin_GNSS;
        SFE_UBLOX_GNSS myGNSS;
        int GNSS_FREQ;
};

#endif // GNSS_H