#ifndef MODEM_H
#define MODEM_H



#define TINY_GSM_RX_BUFFER 1024
#define TINY_GSM_MODEM_SIM7600
// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG Serial

#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

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

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <SoftwareSerial.h>

#define SerialAT Serial1

// Define how you're planning to connect to the internet.
// This is only needed for this example, not in other code.
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

class Modem{
    public:
        Modem();
        ~Modem();
        void setup();
        void process();
        bool connected();
        void print(const char*);
        //StreamDebugger debugger(SerialAT, Serial);
    private:
        TinyGsmClient ntripClient; 
        TinyGsmClient mqttClient;
        TinyGsm* modem;
};
#endif