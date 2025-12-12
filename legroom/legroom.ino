#include <FastLED.h>

#define LED_PIN         2
#define NUM_LEDS        1440
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define MAX_BRIGHTNESS  200

CRGB leds[NUM_LEDS];

void setup() {
  delay(1000);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
}

void loop() {
  static uint8_t hueBase = 0;

  for (int i = 0; i < NUM_LEDS; i++) {
    // Spread the rainbow evenly over the whole strip
    uint8_t hue = hueBase + ((uint32_t)i * 256 / NUM_LEDS);
    leds[i] = CHSV(hue, 255, 255);
  }

  FastLED.show();
  hueBase++;
  delay(20);
}