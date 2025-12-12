void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  Serial.println("USB CDC Serial is working!");
}

void loop() {
  Serial.println("Still alive!");
  delay(1000);
}
