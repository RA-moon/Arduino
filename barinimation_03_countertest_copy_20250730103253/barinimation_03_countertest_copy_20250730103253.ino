#include <Adafruit_NeoPixel.h>

// ====== USER CONFIGURATION ======
#define PIN        17
#define NUM_LEDS   120
#define BRIGHTNESS 80

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// How many LEDs lit per frame
#define WINDOW_SIZE 1

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

void loop() {
  // Slide window from (NUM_LEDS-4) up to (NUM_LEDS+WINDOW_SIZE-1)
  // to wrap around, and keep it always 5 LEDs wide.
  static int pos = 0;
  
  // Turn all LEDs off
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0);
  }

  // Turn on window of 5 LEDs, wrapping around
  for (int i = 0; i < WINDOW_SIZE; i++) {
    int led = (pos + i + NUM_LEDS - 4) % NUM_LEDS; // -4 so we start at 116 (for 120 LEDs)
    strip.setPixelColor(led, strip.Color(255, 255, 255)); // White, change color if needed
  }

  strip.show();
  delay(500); // Adjust speed

  pos++;
  if (pos >= NUM_LEDS) pos = 0; // Loop back to start
}
