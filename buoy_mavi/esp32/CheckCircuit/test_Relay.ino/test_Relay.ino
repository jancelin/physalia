#define pin_Relay 33

void setup(){
  Serial.begin(115200);
  Serial.println("Setup start");
  pinMode(pinRelay, OUTPUT);
}

void loop(){
  Serial.println("pin_Drotek is LOW !");
  digitalWrite(pin_Drotek, LOW);
  delay(1000);
  Serial.println("Drotek is HIGH !");
  digitalWrite(pin_Drotek, HIGH);
  delay(1000);

}
  