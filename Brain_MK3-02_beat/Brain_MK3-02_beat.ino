#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "animtated_circles.h"
#include "animtated_lines.h"
#include "animtated_circles_reversed.h"
#include "animtated_lines_reversed.h"
#include "frame_interpolation.h"
#include "waveform.h"
#include "wave_position.h"
#include "audio_processor.h"
#include "animation_manager.h"

// === Hardware config ===
#define DATA_PIN1         17
#define NUM_LEDS1         120

// Optional 2nd strip ("hair").
// You mentioned the hair data line is on GPIO2; change as needed.
// Keep it disabled for now to free CPU time for audio/FFT.
#define ENABLE_HAIR_STRIP  0
#define DATA_PIN2          2
#define NUM_LEDS2          44

#define LED_FPS           60
#define BRIGHTNESS1       80   // 0..255
#define DELAY_MS          10
#define WAVE_INTERVAL     500
#define AUDIO_INTERVAL    20
#define MAX_ACTIVE_WAVES  10

// Waves are triggered on detected beats. If no beats are detected for a while,
// the fallback timer will still inject occasional waves so the strip doesn't go idle.
#define ENABLE_BEAT_WAVES     1
#define ENABLE_FALLBACK_WAVES 1

Adafruit_NeoPixel strip1(NUM_LEDS1, DATA_PIN1, NEO_GRB + NEO_KHZ800);

#if ENABLE_HAIR_STRIP
Adafruit_NeoPixel strip2(NUM_LEDS2, DATA_PIN2, NEO_GRB + NEO_KHZ800);
#endif

// Wave + pulse logic
static const float decayRate = 0.90f;
unsigned long lastWaveTime = 0;
unsigned long lastAudioTime = 0;


#if ENABLE_HAIR_STRIP
// === Strip 2: Rainbow segment (LEDs 0–31 & 40–43) ===
float rainbowWanderSpeed = 0.002f;
float rainbowScaleSpeed  = 0.002f;
float rainbowPosition    = 0.0f;
float rainbowScale       = 1.0f;
int rainbowScaleDirection = -1;

// === Strip 2: Uniform color with pulse and flicker (LEDs 32–39) ===
const unsigned long RAINBOW_CYCLE_DURATION = 120000UL; // 2 minutes
int fadeBrightness = 0;
int fadeDirection = 1;
#endif

static inline void showStrips() {
  strip1.show();
#if ENABLE_HAIR_STRIP
  strip2.show();
#endif
}

void runLedAnimation() {
  updateAnimationSwitch();
  const auto& frames = getCurrentAnimationFrames();

  // Tell the wave engine how many frames the current animation has.
  setWaveFrameCount((int)frames.size());

  strip1.clear();

  updateWaves();
  const auto& waves = getWaves();

  for (const auto& wave : waves) {
    auto frame = getInterpolatedFrame(
      frames,
      wave.center,
      wave.hue,
      wave.tailWidth,
      wave.noseWidth,
      (int)(BRIGHTNESS1 * brightnessPulse),
      wave.reverse
    );

    for (const auto& f : frame) {
      if (f.ledIndex >= 0 && f.ledIndex < NUM_LEDS1) {
        strip1.setPixelColor((uint16_t)f.ledIndex, f.color);
      }
    }
  }

  // Decay the audio pulse back to baseline.
  brightnessPulse *= decayRate;
  const float minPulse = 60.0f / (float)BRIGHTNESS1;
  if (brightnessPulse < minPulse) brightnessPulse = minPulse;

  const unsigned long now = millis();

#if ENABLE_BEAT_WAVES
  float beatStrength = 0.0f;
  if (consumeBeat(&beatStrength)) {
    if (getWaves().size() < MAX_ACTIVE_WAVES) {
      const uint32_t hue = (uint32_t)random(0, 65536);
      const int8_t speedCtl = (int8_t)constrain((int)(beatStrength * 25.0f) - 5, -10, 10);
      const float nose = 0.8f + (beatStrength * 2.5f);
      const float tail = 1.5f + (beatStrength * 4.0f);
      const bool reverse = (random(0, 100) < 25);
      addWave(hue, speedCtl, nose, tail, reverse);
    }

    // Suppress the fallback timer right after a beat.
    lastWaveTime = now;
  }
#endif

#if ENABLE_FALLBACK_WAVES
  if (now - lastWaveTime >= WAVE_INTERVAL) {
    if (getWaves().size() < MAX_ACTIVE_WAVES) {
      addWave((uint32_t)random(0, 65536), 0, 1.0f, 2.0f);
    }
    lastWaveTime = now;
  }
#endif
}

void setup() {
  delay(300);
  Serial.begin(115200);

  strip1.begin();

#if ENABLE_HAIR_STRIP
  strip2.begin();
#endif

  strip1.setBrightness(BRIGHTNESS1);

#if ENABLE_HAIR_STRIP
  strip2.setBrightness(255);
#endif

  strip1.clear();

#if ENABLE_HAIR_STRIP
  strip2.clear();
#endif
  showStrips();

  resetWaves();

  // Simple entropy seed (works without ADC wiring).
  randomSeed((uint32_t)micros());

  setupI2S(); // In this project build, it's a safe no-op unless you implement real I2S.

  lastWaveTime = millis();
  lastAudioTime = millis();
}

void loop() {
  const unsigned long now = millis();

  if (now - lastAudioTime >= AUDIO_INTERVAL) {
    processAudio();
    lastAudioTime = now;
  }

  runLedAnimation();

#if ENABLE_HAIR_STRIP
  // === Strip 2: Wandering + Scaling Rainbow Section (LEDs 0–31, 40–43) ===
  rainbowPosition += rainbowWanderSpeed;
  if (rainbowPosition > 1.0f) rainbowPosition -= 1.0f;

  rainbowScale += rainbowScaleSpeed * (float)rainbowScaleDirection;
  if (rainbowScale >= 1.0f) {
    rainbowScale = 1.0f;
    rainbowScaleDirection = -1;
  } else if (rainbowScale <= 0.1f) {
    rainbowScale = 0.1f;
    rainbowScaleDirection = 1;
  }

  const int rainbowLeds[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
   16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
   40,41,42,43
  };
  const int numRainbowLeds = (int)(sizeof(rainbowLeds) / sizeof(rainbowLeds[0]));

  for (int i = 0; i < numRainbowLeds; i++) {
    const float posRatio = (float)i / (float)(numRainbowLeds - 1);
    float huePos = rainbowPosition + (posRatio * rainbowScale);
    if (huePos > 1.0f) huePos -= 1.0f;

    const uint16_t hue16 = (uint16_t)(huePos * 65535.0f);
    strip2.setPixelColor((uint16_t)rainbowLeds[i], strip2.ColorHSV(hue16, 255, 255));
  }

  // === Strip 2: Uniform Color with Pulse & Flicker (LEDs 32–39) ===
  const unsigned long elapsedCycleTime = now % RAINBOW_CYCLE_DURATION;
  const float cycleProgress = (float)elapsedCycleTime / (float)RAINBOW_CYCLE_DURATION;
  const uint8_t solidHue8 = (uint8_t)(cycleProgress * 255.0f);
  const uint16_t solidHue16 = (uint16_t)solidHue8 << 8;

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
    effectiveBrightness += random(-10, 10);
    effectiveBrightness = constrain(effectiveBrightness, 0, 255);
  }

  const uint32_t fadeColor = strip2.ColorHSV(solidHue16, 255, (uint8_t)effectiveBrightness);
  for (int i = 32; i <= 39; i++) {
    strip2.setPixelColor((uint16_t)i, fadeColor);
  }

#endif // ENABLE_HAIR_STRIP

  showStrips();
  delay(DELAY_MS);
  yield();
}
