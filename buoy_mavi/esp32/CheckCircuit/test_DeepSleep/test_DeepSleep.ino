// Source from : https://fr.thesouthshow.com/21996-Interfacing-Photoresistor-With-ESP32-74
// Testing Deep Sleep whit ESP32
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP  30

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR double lastSleep = 0;

long firstFix = 0;
long lastFix = 0;
long TIME_FIX_TO_RECORD = 3000; // 3sec

void print_wakeup_reason(){
   esp_sleep_wakeup_cause_t source_reveil;

   source_reveil = esp_sleep_get_wakeup_cause();

   switch(source_reveil){
      case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Réveil causé par un signal externe avec RTC_IO"); break;
      case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Réveil causé par un signal externe avec RTC_CNTL"); break;
      case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Réveil causé par un timer"); break;
      case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Réveil causé par un touchpad"); break;
      default : Serial.printf("Réveil pas causé par le Deep Sleep: %d\n",source_reveil); break;
   }
}

bool state_fix = false;
long nb_millisecond_recorded = 0;

void setup(){
   Serial.begin(115200);

   ++bootCount;
   Serial.println("----------------------");
   Serial.println(String(bootCount)+ "eme Boot ");

   //Affiche la raison du réveil
   print_wakeup_reason();

   //Configuration du timer
   esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

}

void loop(){
   // SI on récupère pendant 10sec le FIX on est bon ! 
   double now = millis();
   Serial.println("now = " + String(now) + " - nb_millisecond_recorded = " + String(nb_millisecond_recorded));

   // SIMULATION - On récupère la valeur du state_fix... aprés 5sec
   if ( now - firstFix > 5000 ) {
      state_fix = true;
   }
   // SIMULATION - Aprés 7 seconde on perd le signal pendant 7 secondes
   if ( now > 6500 && now < 15000) {
      Serial.println("Test de perte du Fix aprés 7 secondes ET jusqu'à 15 sec. ");
      state_fix = false;
      firstFix = 0;
   }
   
   if (!state_fix)
      nb_millisecond_recorded = 0;
   else 
      ++nb_millisecond_recorded;

   if ( state_fix && nb_millisecond_recorded > TIME_FIX_TO_RECORD ){
      Serial.println("Record quality FIX during done, we can sleep at " + String(now));
      Serial.println("ESP32 réveillé dans " + String(TIME_TO_SLEEP) + " seconds");

      esp_deep_sleep_start();
   }

}