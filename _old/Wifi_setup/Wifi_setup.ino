#include <WiFi.h>
#include <ArduinoOTA.h>

const char* ssid = "Unicorn";
const char* password = "1Einhorn";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
}
