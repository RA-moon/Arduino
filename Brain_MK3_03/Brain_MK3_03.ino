#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

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

#define BRIGHTNESS1       80   // baseline (0..255)
#define DELAY_MS          10
#define WAVE_INTERVAL     500
#define AUDIO_INTERVAL    20
#define MAX_ACTIVE_WAVES  10

// Beat-synced brightness envelope:
// - On beat: brightness = 255
// - Then decays to BRIGHTNESS1 over the *average* beat time
#define BEAT_DECAY_MIN_MS   160
#define BEAT_DECAY_MAX_MS  1500
#define BEAT_DECAY_EASE_OUT   1   // 1 = quadratic ease-out, 0 = linear

// Waves are triggered on detected beats. If no beats are detected for a while,
// the fallback timer will still inject occasional waves so the strip doesn't go idle.
#define ENABLE_BEAT_WAVES     1
#define ENABLE_FALLBACK_WAVES 1

// Serial debug print on every beat
#define DEBUG_BEAT_TIMING     0

Adafruit_NeoPixel strip1(NUM_LEDS1, DATA_PIN1, NEO_GRB + NEO_KHZ800);

#if ENABLE_HAIR_STRIP
Adafruit_NeoPixel strip2(NUM_LEDS2, DATA_PIN2, NEO_GRB + NEO_KHZ800);
#endif

static uint32_t lastWaveTime = 0;
static uint32_t lastAudioTime = 0;

// Beat envelope state
static uint32_t lastBeatMs = 0;

static inline void showStrips() {
  strip1.show();
#if ENABLE_HAIR_STRIP
  strip2.show();
#endif
}

static inline float clamp01(float x) {
  if (x < 0.0f) return 0.0f;
  if (x > 1.0f) return 1.0f;
  return x;
}

static inline float beatEnvelope(float beatPeriodMs, uint32_t nowMs) {
  if (lastBeatMs == 0) return 0.0f;

  const uint32_t dt = nowMs - lastBeatMs;
  if (dt >= (uint32_t)beatPeriodMs) return 0.0f;

  float e = 1.0f - ((float)dt / beatPeriodMs); // 1..0
#if BEAT_DECAY_EASE_OUT
  e *= e;
#endif
  return clamp01(e);
}

void runLedAnimation() {
  const uint32_t now = millis();

#if ENABLE_BEAT_WAVES
  float beatStrength = 0.0f;
  const bool beatEvent = consumeBeat(&beatStrength);
  if (beatEvent) {
    lastBeatMs = now;

    // Suppress the fallback timer right after a beat.
    lastWaveTime = now;

#if DEBUG_BEAT_TIMING
    Serial.printf("Beat: avg=%.0fms (%.1f BPM) strength=%.2f\n",
                  getAverageBeatIntervalMs(), getAverageBpm(), beatStrength);
#endif
  }
#else
  const bool beatEvent = false;
  const float beatStrength = 0.0f;
#endif

  // Use the tempo estimate from the audio module.
  float beatPeriodMs = getAverageBeatIntervalMs();
  if (beatPeriodMs < (float)BEAT_DECAY_MIN_MS) beatPeriodMs = (float)BEAT_DECAY_MIN_MS;
  if (beatPeriodMs > (float)BEAT_DECAY_MAX_MS) beatPeriodMs = (float)BEAT_DECAY_MAX_MS;

  const float env = beatEnvelope(beatPeriodMs, now);

  // Brightness for this frame: 255 on beat -> BRIGHTNESS1 at the end of the beat period.
  int frameBrightness = BRIGHTNESS1 + (int)lroundf((255.0f - (float)BRIGHTNESS1) * env);
  frameBrightness = constrain(frameBrightness, 0, 255);

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
      frameBrightness,
      wave.reverse
    );

    for (const auto& f : frame) {
      if (f.ledIndex >= 0 && f.ledIndex < NUM_LEDS1) {
        strip1.setPixelColor((uint16_t)f.ledIndex, f.color);
      }
    }
  }

#if ENABLE_BEAT_WAVES
  // Beat-triggered wave injection.
  if (beatEvent) {
    if (getWaves().size() < MAX_ACTIVE_WAVES) {
      const uint32_t hue = (uint32_t)random(0, 65536);
      const int8_t speedCtl = (int8_t)constrain((int)(beatStrength * 25.0f) - 5, -10, 10);
      const float nose = 0.8f + (beatStrength * 2.5f);
      const float tail = 1.5f + (beatStrength * 4.0f);
      const bool reverse = (random(0, 100) < 25);
      addWave(hue, speedCtl, nose, tail, reverse);
    }
  }
#endif

#if ENABLE_FALLBACK_WAVES
  // Fallback wave injection so the strip doesn't go idle without music.
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

  // Keep NeoPixel's internal brightness scaler at full.
  // Beat pulsing is handled by the per-frame brightness parameter.
  strip1.setBrightness(255);
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

  setupI2S();

  lastWaveTime = millis();
  lastAudioTime = millis();
}

void loop() {
  const uint32_t now = millis();

  if (now - lastAudioTime >= AUDIO_INTERVAL) {
    processAudio();
    lastAudioTime = now;
  }

  runLedAnimation();

#if ENABLE_HAIR_STRIP
  // Hair rendering will be re-enabled later.
#endif

  showStrips();
  delay(DELAY_MS);
  yield();
}
