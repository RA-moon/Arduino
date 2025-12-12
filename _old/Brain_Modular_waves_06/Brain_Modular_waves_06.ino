#include <Adafruit_NeoPixel.h>
#include "animtated_circles.h"
#include "frame_interpolation.h"
#include "waveform.h"
#include "wave_position.h"

#define PIN         5
#define NUM_LEDS    120
#define BRIGHTNESS  80
#define DELAY_MS    30
#define WAVE_INTERVAL 5000  // Launch new wave every 500ms

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

unsigned long lastWaveTime = 0;

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  resetWaves();
  randomSeed(analogRead(0));  // Ensure randomness
}

void loop() {
  static std::vector<std::vector<int>> frames = getAnimationFrames();
  strip.clear();

  // Update and render waves
  updateWaves();
  const auto& waves = getWaves();
  for (const auto& wave : waves) {
    auto frame = getInterpolatedFrame(frames, wave.center, wave.hue, wave.tailWidth, wave.noseWidth, BRIGHTNESS, wave.reverse);
    for (const auto& f : frame) {
      strip.setPixelColor(f.ledIndex, f.color);
    }
  }

  strip.show();
  delay(DELAY_MS);

  // Add a new random wave every 500ms
  if (millis() - lastWaveTime >= WAVE_INTERVAL) {
    uint32_t hue = random(0, 65536);



    addWave(hue, 0, 0.5, 2.0, false);
    lastWaveTime = millis();
  }
}
