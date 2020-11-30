int MAGNETIC_PIN = 2;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(MAGNETIC_PIN, INPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  int state = digitalRead(MAGNETIC_PIN);
  if(state == HIGH) {
    Serial.println("Lid Removed");
    delay(3000);
  }
}
