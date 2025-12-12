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

#define DATA_PIN1         17
#define NUM_LEDS1         120
#define DATA_PIN2         16
#define NUM_LEDS2         44

#define LED_TYPE           WS2812B
#define COLOR_ORDER        GRB
#define BRIGHTNESS1        80
#define DELAY_MS           10
#define WAVE_INTERVAL      500
#define AUDIO_INTERVAL     20
#define MAX_ACTIVE_WAVES   10

CRGB leds1[NUM_LEDS1];
CRGB leds2[NUM_LEDS2];

const float decayRate = 0.9;
unsigned long lastWaveTime = 0;
unsigned long lastAudioTime = 0;

void runLedAnimation() {
  updateAnimationSwitch();
  const auto& frames = getCurrentAnimationFrames();
  fill_solid(leds1, NUM_LEDS1, CRGB::Black);

  updateWaves();
  const auto& waves = getWaves();
  for (const auto& wave : waves) {
    auto frame = getInterpolatedFrame(
      frames,
      wave.center,
      wave.hue,
      wave.tailWidth,
      wave.noseWidth,
      BRIGHTNESS1 * brightnessPulse,
      wave.reverse
    );

    for (const auto& f : frame) {
      if (f.ledIndex < NUM_LEDS1) {
        leds1[f.ledIndex] = f.color;
      }
    }
  }

  brightnessPulse *= decayRate;
  float minPulse = float(60) / BRIGHTNESS1;
  if (brightnessPulse < minPulse) brightnessPulse = minPulse;

  unsigned long now = millis();
  if (now - lastWaveTime >= WAVE_INTERVAL) {
    if (getWaves().size() < MAX_ACTIVE_WAVES) {
      addWave(random(0, 65536), 0, 1, 2);
      lastWaveTime = now;
    }
  }
}

void setup() {
  delay(1000);
  FastLED.addLeds<LED_TYPE, DATA_PIN1, COLOR_ORDER>(leds1, NUM_LEDS1).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, DATA_PIN2, COLOR_ORDER>(leds2, NUM_LEDS2).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS1);
  FastLED.clear();
  FastLED.show();
  resetWaves();
  randomSeed(analogRead(0));
  setupI2S();
  Serial.begin(115200);
}

void loop() {
  unsigned long now = millis();

  if (now - lastAudioTime >= AUDIO_INTERVAL) {
    processAudio();
    lastAudioTime = now;
  }

  runLedAnimation();

  // === Strip 2: Full rainbow ===
  uint16_t rainbowHue = millis() / 10;
  for (int i = 0; i < NUM_LEDS2; i++) {
    leds2[i] = CHSV((rainbowHue + i * 8) % 256, 255, 255);
  }

  FastLED[0].showLeds(BRIGHTNESS1);
  FastLED[1].showLeds(255);
  delay(DELAY_MS);
}
