#ifndef MODEM_H
#define MODEM_H

#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_DEBUG Serial

#include <Arduino.h>
#include <TinyGsmClient.h>


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
        //StreamDebugger debugger(SerialAT, Serial);
        TinyGsmClient ntripClient; 
    private:
};

#endif