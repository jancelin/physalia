#ifndef DATA_H
#define DATA_H

#include <ArduinoJson.h>
#include <SparkFun_u-blox_GNSS_v3.h> //http://librarymanager/All#SparkFun_u-blox_GNSS

/* Classe de formatage en JSON (...ou autre) de la trame ubxDataStruc */

class DataFormat{
    public:
        DataFormat(UBX_NAV_PVT_data_t *ubxDataStruct);
        ~DataFormat();
        StaticJsonDocument<256> getDocJson();

    private:
        UBX_NAV_PVT_data_t *ubxDataStruct;
};

#endif // DATA_H