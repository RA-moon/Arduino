#include <Adafruit_NeoPixel.h>
#include "animtated_circles.h"

// ====== CONFIGURATION ======
#define PIN         5
#define NUM_LEDS    120
#define BRIGHTNESS  80

#define WAVE_WIDTH  2.0      // Affects how many rings are lit
#define STEP_SIZE   0.2      // Speed of wave progression
#define DELAY_MS    30       // Time between frames

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

float waveCenter = 0.0;
int waveCycle = 0;

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

float getIntensity(float distance, float width) {
  float norm = distance / width;
  return exp(-norm * norm);
}

void loop() {
  static std::vector<std::vector<int>> rings = getAnimationFrames(); // each is a ring
  int totalRings = rings.size();

  strip.clear();

  for (int r = 0; r < totalRings; r++) {
    float dist = r - waveCenter;
    float intensity = getIntensity(dist, WAVE_WIDTH);
    if (intensity < 0.01) continue; // Skip near-zero

    uint8_t value = BRIGHTNESS * intensity;
    uint32_t hue = fmod((waveCycle + r) * 65536.0 / 50, 65536);
    uint32_t color = strip.ColorHSV(hue, 255, value);

    for (int led : rings[r]) {
      if (led >= 0 && led < NUM_LEDS) {
        strip.setPixelColor(led, color);
      }
    }
  }

  strip.show();
  delay(DELAY_MS);

  waveCenter += STEP_SIZE;
  if (waveCenter > rings.size() + WAVE_WIDTH) {
    waveCenter = -WAVE_WIDTH;  // restart before first ring
    waveCycle++;               // advance color cycle
  }
}
