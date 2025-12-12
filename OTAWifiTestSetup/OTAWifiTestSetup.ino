#include <WiFi.h>
#include <ArduinoOTA.h>

const char* ssid = "MIX 4";
const char* password = "Ramun123";


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");

  // Optional: OTA Hostname and Password
  ArduinoOTA.setHostname("esp32-c6");
  // ArduinoOTA.setPassword("1234"); // Optional password protection

  ArduinoOTA.onStart([]() {
    Serial.println("Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  ArduinoOTA.handle();
}
