#include <FastLED.h>

#define DATA_PIN1     17
#define NUM_LEDS1     120
#define BRIGHTNESS    80
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB

CRGB leds[NUM_LEDS1];

bool ledOn = false;
unsigned long lastUpdate = 0;
const unsigned long interval = 500;  // ms

void setup() {
  delay(1000);
  FastLED.addLeds<LED_TYPE, DATA_PIN1, COLOR_ORDER>(leds, NUM_LEDS1);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
}

void loop() {
  unsigned long now = millis();
  if (now - lastUpdate >= interval) {
    lastUpdate = now;

    leds[5] = ledOn ? CRGB::White : CRGB::Black;
    ledOn = !ledOn;

    FastLED.show();
  }
}
