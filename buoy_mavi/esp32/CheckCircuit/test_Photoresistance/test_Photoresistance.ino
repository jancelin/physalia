#define LDR 2  // composante photoresistor sur la pin A1

unsigned int value;
 
void setup() {
   // initialise la communication avec le PC
   Serial.begin(115200);

   // initialise les broches
   pinMode(LDR, INPUT);
}
 
void loop() {
   // mesure la tension sur la broche A1
   value = analogRead(LDR);
   Serial.println(value);

   delay(200);
}