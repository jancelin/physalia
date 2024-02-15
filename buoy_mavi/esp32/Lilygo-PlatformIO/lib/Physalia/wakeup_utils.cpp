// wakeup_utils.cpp
#include "wakeup_utils.h"

void print_wakeup_reason() {
    // Code pour imprimer la raison du réveil
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