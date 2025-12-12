// Combined ESP32-C6 Sketch for Two Independent LED Strips
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

// === CONFIGURATION FOR BRAIN STRIP ===
#define BRAIN_DATA_PIN         17
#define BRAIN_NUM_LEDS         120
#define BRAIN_LED_TYPE         WS2812B
#define BRAIN_COLOR_ORDER      GRB
#define BRAIN_BRIGHTNESS       80
#define WAVE_INTERVAL          500
#define AUDIO_INTERVAL         20
#define MAX_ACTIVE_WAVES       10

CRGB ledsBrain[BRAIN_NUM_LEDS];

const float decayRate = 0.9;
unsigned long lastWaveTime = 0;
unsigned long lastAudioTime = 0;

// === CONFIGURATION FOR HAIR STRIP ===
#define HAIR_DATA_PIN          16
#define HAIR_NUM_LEDS          1440
#define HAIR_LED_TYPE          WS2812B
#define HAIR_COLOR_ORDER       GRB
#define HAIR_BRIGHTNESS        120
#define HAIR_FRAME_INTERVAL    30
#define COLOR_CYCLE_DURATION   1800000UL // 30 min

CRGB ledsHair[HAIR_NUM_LEDS];

uint8_t hairHueOffset = 0;
int hairFadeBrightness = 0;
int hairFadeDirection = 1;
unsigned long hairColorCycleStart = 0;
unsigned long lastHairFrameTime = 0;

// === BRAIN WAVE ANIMATION ===
void runBrainLedAnimation() {
  updateAnimationSwitch();
  const auto& frames = getCurrentAnimationFrames();

  // Clear only brain strip
  for (int i = 0; i < BRAIN_NUM_LEDS; i++) {
    ledsBrain[i] = CRGB::Black;
  }

  updateWaves();
  const auto& waves = getWaves();
  for (const auto& wave : waves) {
    auto frame = getInterpolatedFrame(
      frames,
      wave.center,
      wave.hue,
      wave.tailWidth,
      1.0,
      BRAIN_NUM_LEDS,
      false
    );
    for (int i = 0; i < BRAIN_NUM_LEDS; ++i) {
      ledsBrain[i] += frame[i].color;
    }
  }
}

// === HAIR SIMPLE FADE/RAINBOW ===
void runHairLedAnimation() {
  unsigned long currentTime = millis();
  if (currentTime - lastHairFrameTime < HAIR_FRAME_INTERVAL) return;
  lastHairFrameTime = currentTime;

  hairHueOffset++;
  for (int i = 0; i < HAIR_NUM_LEDS; i++) {
    ledsHair[i] = CHSV((hairHueOffset + i / 10) % 255, 255, hairFadeBrightness);
  }

  hairFadeBrightness += hairFadeDirection * 5;
  if (hairFadeBrightness >= HAIR_BRIGHTNESS || hairFadeBrightness <= 0) {
    hairFadeDirection *= -1;
  }
}

void setup() {
  FastLED.addLeds<BRAIN_LED_TYPE, BRAIN_DATA_PIN, BRAIN_COLOR_ORDER>(ledsBrain, BRAIN_NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<HAIR_LED_TYPE, HAIR_DATA_PIN, HAIR_COLOR_ORDER>(ledsHair, HAIR_NUM_LEDS).setCorrection(TypicalLEDStrip);

  FastLED.setBrightness(255);  // Use full brightness; animation controls visual intensity

  hairColorCycleStart = millis();
  lastHairFrameTime = millis();
}

void loop() {
  runHairLedAnimation();
  runBrainLedAnimation();
  FastLED.show();
}
