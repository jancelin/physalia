#define pin_Relay 32

void setup(){
  Serial.begin(115200);
  Serial.println("Setup start");
  pinMode(pin_Relay, OUTPUT);
}

void loop(){
  Serial.println("pin relay is LOW !");
  digitalWrite(pin_Relay, LOW);
  delay(5000);
  Serial.println("pin relay is HIGH !");
  digitalWrite(pin_Relay, HIGH);
  delay(10000);

}
  