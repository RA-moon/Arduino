
#include <Adafruit_NeoPixel.h>
#include "animtated_circles.h"
#include "frame_interpolation.h"
#include "waveform.h"
#include "wave_position.h"

#define PIN         5
#define NUM_LEDS    120
#define BRIGHTNESS  80
#define DELAY_MS    30
#define WAVE_INTERVAL 2000

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);
unsigned long lastWaveTime = 0;

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  resetWaves();
  randomSeed(analogRead(0));
}

void loop() {
  static std::vector<std::vector<int>> frames = getAnimationFrames();
  strip.clear();

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

  if (millis() - lastWaveTime >= WAVE_INTERVAL && getWaves().size() < 7) {
    uint32_t hue = random(0, 65536);
    

    addWave(hue, -10, 1, 3.0 );
    lastWaveTime = millis();
  }
}
