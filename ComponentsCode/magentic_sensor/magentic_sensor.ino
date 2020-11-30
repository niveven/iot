int MAGNETIC_PIN_INPUT_TO_ARDUINO = 2;
int MAGNETIC_PIN_OUTPUT_FROM_ARDUINO = 4;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(MAGNETIC_PIN_INPUT_TO_ARDUINO, INPUT);
  pinMode(MAGNETIC_PIN_OUTPUT_FROM_ARDUINO, OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  int state = digitalRead(MAGNETIC_PIN_INPUT_TO_ARDUINO);
  if(state == HIGH) {
    Serial.println("Arsuino got input from sensor");
    digitalWrite(MAGNETIC_PIN_OUTPUT_FROM_ARDUINO, HIGH);
  }
  if(state == LOW) {
    digitalWrite(MAGNETIC_PIN_OUTPUT_FROM_ARDUINO, LOW);
  }
}
