#include <FastLED.h>

#define DATA_PIN      17
#define NUM_LEDS      120
#define BRIGHTNESS    80
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB

CRGB leds[NUM_LEDS];

const uint32_t interval = 200;   // ms (Blink-Tempo)
uint32_t lastUpdate = 0;

uint16_t idx = 0;       // aktuelle LED (0..NUM_LEDS-1)
bool phaseOn = true;    // an/aus-Phase pro LED

void setup() {
  delay(1000);
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true); // clear + show
}

void loop() {
  uint32_t now = millis();
  if (now - lastUpdate < interval) return;
  lastUpdate = now;

  FastLED.clear();              // alles aus
  if (phaseOn) {
    leds[idx] = CRGB::White;    // diese LED an
    phaseOn = false;
  } else {
    // bleibt aus (clear), dann weiter zur nächsten
    phaseOn = true;
    idx = (idx + 1) % NUM_LEDS;
  }

  FastLED.show();
}
