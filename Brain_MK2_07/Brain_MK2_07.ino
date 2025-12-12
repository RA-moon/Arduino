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

// === Strip 2: Rainbow segment (LEDs 0–31 & 40–43) ===
float rainbowWanderSpeed = 0.002;
float rainbowScaleSpeed = 0.002;
float rainbowPosition = 0.0;
float rainbowScale = 1.0;
int rainbowScaleDirection = -1;

// === Strip 2: Uniform color with pulse and flicker (LEDs 32–39) ===
const unsigned long RAINBOW_CYCLE_DURATION = 120000UL;  // 2 minutes
int fadeBrightness = 0;
int fadeDirection = 1;

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

  // === Strip 2: Wandering + Scaling Rainbow Section (LEDs 0–31, 40–43) ===
  rainbowPosition += rainbowWanderSpeed;
  if (rainbowPosition > 1.0) rainbowPosition -= 1.0;

  rainbowScale += rainbowScaleSpeed * rainbowScaleDirection;
  if (rainbowScale >= 1.0) {
    rainbowScale = 1.0;
    rainbowScaleDirection = -1;
  } else if (rainbowScale <= 0.1) {
    rainbowScale = 0.1;
    rainbowScaleDirection = 1;
  }

  const int rainbowLeds[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
   16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
   40,41,42,43
  };
  const int numRainbowLeds = sizeof(rainbowLeds) / sizeof(rainbowLeds[0]);

  for (int i = 0; i < numRainbowLeds; i++) {
    float posRatio = (float)i / (numRainbowLeds - 1);
    float huePos = rainbowPosition + (posRatio * rainbowScale);
    if (huePos > 1.0) huePos -= 1.0;
    leds2[rainbowLeds[i]] = CHSV(huePos * 255, 255, 255);
  }

  // === Strip 2: Uniform Color with Pulse & Flicker (LEDs 32–39) ===
  unsigned long elapsedCycleTime = now % RAINBOW_CYCLE_DURATION;
  float cycleProgress = (float)elapsedCycleTime / RAINBOW_CYCLE_DURATION;
  uint8_t solidHue = cycleProgress * 255;

  // Brightness pulse logic
  fadeBrightness += fadeDirection * 5;
  if (fadeBrightness >= 255) {
    fadeBrightness = 255;
    fadeDirection = -1;
  } else if (fadeBrightness <= 170) {
    fadeBrightness = 170;
    fadeDirection = 1;
  }

  // Optional flickering
  int effectiveBrightness = fadeBrightness;
  if (fadeBrightness > 80 && fadeBrightness < 180) {
    effectiveBrightness += random(-100, 1000);
    effectiveBrightness = constrain(effectiveBrightness, 0, 255);
  }

  CRGB fadeColor = CHSV(solidHue, 255, effectiveBrightness);
  for (int i = 32; i <= 39; i++) {
    leds2[i] = fadeColor;
  }

  // Show LEDs
  FastLED[0].showLeds(BRIGHTNESS1);
  FastLED[1].showLeds(255);
  delay(DELAY_MS);
}
