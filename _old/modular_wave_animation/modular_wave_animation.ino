
#include <Adafruit_NeoPixel.h>
#include "animtated_circles.h"
#include "frame_interpolation.h"
#include "waveform.h"
#include "wave_position.h"

#define PIN         5
#define NUM_LEDS    120
#define BRIGHTNESS  80
#define DELAY_MS    30
#define WAVE_REVERSE false

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  resetWave();
}

void loop() {
  static std::vector<std::vector<int>> frames = getAnimationFrames();
  strip.clear();

  float waveCenter = getNextWavePosition();
  uint32_t hue = getWaveHue();

  auto waveFrame = getInterpolatedFrame(frames, waveCenter, hue, 3.0, 1.0, BRIGHTNESS, WAVE_REVERSE);

  for (auto& f : waveFrame) {
    strip.setPixelColor(f.ledIndex, f.color);
  }

  strip.show();
  delay(DELAY_MS);
}
