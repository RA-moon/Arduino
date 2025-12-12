#include <FastLED.h>

#define LED_TYPE     WS2812B      // Change if your LEDs are different
#define COLOR_ORDER  GRB
#define DATA_PIN     2            // <— your data pin
#define NUM_LEDS     60           // Set this to match your strip
#define BRIGHTNESS   80
#define TAIL_FADE    40           // 0..255; higher = shorter tail
#define FRAME_DELAY  20           // ms per frame (lower = faster)

CRGB leds[NUM_LEDS];

int pos  = 0;
int dir  = 1;                     // 1 = forward, -1 = backward
uint8_t hue = 0;

void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  // Dim all pixels to create a trailing tail
  fadeToBlackBy(leds, NUM_LEDS, TAIL_FADE);

  // Draw the head of the comet
  leds[pos] = CHSV(hue, 255, 255);

  FastLED.show();
  delay(FRAME_DELAY);

  // Move and bounce at the ends
  pos += dir;
  if (pos <= 0 || pos >= NUM_LEDS - 1) {
    dir = -dir;
    pos = constrain(pos, 0, NUM_LEDS - 1);
  }

  hue++; // slowly cycle color
}
