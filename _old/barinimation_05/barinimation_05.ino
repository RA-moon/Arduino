#include <Adafruit_NeoPixel.h>
#include "animtated_circles.h"

// ====== USER CONFIGURATION ======
#define PIN        5
#define NUM_LEDS   120
#define BRIGHTNESS 80

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// Number of simultaneous waves
#define NUM_WAVES     4
#define FRAME_OFFSET  3
#define DELAY_MS      150

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

void loop() {
  static std::vector<std::vector<int>> frames = getAnimationFrames();
  static int baseFrameIndex = 0;
  int totalFrames = frames.size();

  // Clear all LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0);
  }

  // Render each wave
  for (int wave = 0; wave < NUM_WAVES; wave++) {
    int frameIndex = (baseFrameIndex + wave * FRAME_OFFSET) % totalFrames;
    std::vector<int>& frame = frames[frameIndex];

    // Compute rainbow color
    uint32_t color = strip.ColorHSV((wave * 65536L / NUM_WAVES), 255, 255); // rainbow hue

    for (int led : frame) {
      if (led >= 0 && led < NUM_LEDS) {
        strip.setPixelColor(led, color);
      }
    }
  }

  strip.show();
  delay(DELAY_MS);

  baseFrameIndex = (baseFrameIndex + 1) % totalFrames;
}
