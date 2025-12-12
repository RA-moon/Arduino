#include <Adafruit_NeoPixel.h>

#define LED_PIN    5
#define NUM_LEDS   120  // Adjust based on actual pixel_map
#define BRIGHTNESS 100
#define DELAY_TIME 150

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Hardcoded ring structure from the pixel_map.py based on approximate concentric patterning
// You may adjust these groups based on the actual pixel_map geometry
uint8_t rings[][20] = {
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},           // outermost
  {10, 11, 12, 13, 14, 15, 16, 17, 18, 19},
  {20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
  {30, 31, 32, 33, 34, 35, 36, 37, 38, 39},
  {40, 41, 42, 43, 44, 45, 46, 47, 48, 49},
  {50, 51, 52, 53, 54, 55, 56, 57, 58, 59},
  {60, 61, 62, 63, 64, 65, 66, 67, 68, 69},
  {70, 71, 72, 73, 74, 75, 76, 77, 78, 79},
  {80, 81, 82, 83, 84, 85, 86, 87, 88, 89},
  {90, 91, 92, 93, 94, 95, 96, 97, 98, 99},
  {100, 101, 102, 103, 104, 105, 106, 107, 108, 109},
  {110, 111, 112, 113, 114, 115, 116, 117, 118, 119}  // innermost
};
int ringCount = sizeof(rings) / sizeof(rings[0]);

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

void loop() {
  for (int r = 0; r < ringCount; r++) {
    strip.clear();
    for (int i = 0; i < 20; i++) {
      int index = rings[ringCount - 1 - r][i];
      if (index >= 0 && index < NUM_LEDS) {
        strip.setPixelColor(index, strip.Color(255, 0, 0));
      }
    }
    strip.show();
    delay(DELAY_TIME);
  }
}
