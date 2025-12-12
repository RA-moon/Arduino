#include <FastLED.h>
#include "animtated_circles.h"
#include "animtated_lines.h"
#include "animtated_circles_reversed.h"
#include "animtated_lines_reversed.h"
#include "frame_interpolation.h"
#include "waveform.h"
#include "wave_position.h"
#include "audio_processor.h"
#include "animation_manager.h"


#define DATA_PIN         5
#define NUM_LEDS         120
#define LED_TYPE         WS2812B
#define COLOR_ORDER      GRB
#define BRIGHTNESS       80
#define DELAY_MS         10
#define WAVE_INTERVAL    500
#define AUDIO_INTERVAL   20
#define MAX_ACTIVE_WAVES  10

CRGB leds[NUM_LEDS];

const float decayRate = 0.9;
unsigned long lastWaveTime = 0;
unsigned long lastAudioTime = 0;

void runLedAnimation() {
  updateAnimationSwitch();
  const auto& frames = getCurrentAnimationFrames();
  FastLED.clear();

  updateWaves();
  const auto& waves = getWaves();
  for (const auto& wave : waves) {
    auto frame = getInterpolatedFrame(
      frames,
      wave.center,
      wave.hue,
      wave.tailWidth,
      wave.noseWidth,
      BRIGHTNESS * brightnessPulse,
      wave.reverse
    );

    for (const auto& f : frame) {
      leds[f.ledIndex] = f.color;
    }
  }

  brightnessPulse *= decayRate;
  float minPulse = float(60) / BRIGHTNESS;
  if (brightnessPulse < minPulse) brightnessPulse = minPulse;

  FastLED.show();
  delay(DELAY_MS);

  unsigned long now = millis();
  if (now - lastWaveTime >= WAVE_INTERVAL) {
    if (getWaves().size() < MAX_ACTIVE_WAVES) {
      addWave(random(0, 65536), 0, 1, 2);
      lastWaveTime = now;
    }
  }
}

void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  resetWaves();
  randomSeed(analogRead(0));
  setupI2S();  // from audio_processor.cpp
  Serial.begin(115200);
}

void loop() {
  unsigned long now = millis();

  if (now - lastAudioTime >= AUDIO_INTERVAL) {
    processAudio();  // updates brightnessPulse only
    lastAudioTime = now;
  }

  runLedAnimation();
}
