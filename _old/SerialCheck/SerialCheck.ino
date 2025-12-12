void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);  // Wait up to 3 sec for Serial connection
  Serial.println("Hello from ESP32-C6!");
}

void loop() {
  Serial.println("Still running...");
  delay(1000);
}
