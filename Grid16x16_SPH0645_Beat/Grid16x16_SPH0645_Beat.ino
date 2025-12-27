#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <vector>

#include "frame_interpolation.h"
#include "wave_position.h"
#include "audio_processor.h"

// =====================================================
// 16x16 GRID TEST SKETCH (SPH0645 + FFT BEAT)
// - Uses the same wave engine + interpolation as the brain project.
// - Frames are generated from a regular 16x16 grid (no brain mapping).
// - Later compatibility: replace `gridFrames` with your real animation frames.
// =====================================================

// === Hardware config ===
#define DATA_PIN1         17

#define GRID_W            16
#define GRID_H            16
#define GRID_SERPENTINE   0   // 0 = linear rows, 1 = serpentine rows

#define NUM_LEDS1         (GRID_W * GRID_H)

// Optional 2nd strip ("hair") — kept for later compatibility.
// (Disabled for this grid test.)
#define ENABLE_HAIR_STRIP  0
#define DATA_PIN2          2
#define NUM_LEDS2          44

// Baseline brightness (this is the MIN value the beat envelope decays to).
#define BRIGHTNESS1        80   // 0..255

#define DELAY_MS           10
#define AUDIO_INTERVAL     20

// Waves: only on beat; fallback if no beat for a while
#define NO_BEAT_FALLBACK_MS 800
#define MAX_ACTIVE_WAVES     10

// Beat-synced brightness envelope:
// - On beat: brightness = 255
// - Then decays to BRIGHTNESS1 over the *average* beat time
#define BEAT_DECAY_MIN_MS    160
#define BEAT_DECAY_MAX_MS   1500
#define BEAT_DECAY_EASE_OUT    1   // 1 = quadratic ease-out, 0 = linear

#define ENABLE_BEAT_WAVES      1
#define ENABLE_FALLBACK_WAVES  1

#define DEBUG_BEAT_TIMING      0

Adafruit_NeoPixel strip1(NUM_LEDS1, DATA_PIN1, NEO_GRB + NEO_KHZ800);

// Timing
static uint32_t lastWaveTime = 0;
static uint32_t lastAudioTime = 0;

// Beat envelope state
static uint32_t lastBeatMs = 0;

// Frames for the grid (each frame = one row)
static std::vector<std::vector<int>> gridFrames;

static inline float clamp01(float v) {
  if (v < 0.0f) return 0.0f;
  if (v > 1.0f) return 1.0f;
  return v;
}

static inline int xyToIndex(int x, int y) {
#if GRID_SERPENTINE
  if (y & 1) {
    return y * GRID_W + (GRID_W - 1 - x);
  } else {
    return y * GRID_W + x;
  }
#else
  return y * GRID_W + x;
#endif
}

static void buildGridFrames() {
  gridFrames.clear();
  gridFrames.resize(GRID_H);

  for (int y = 0; y < GRID_H; y++) {
    gridFrames[y].reserve(GRID_W);
    for (int x = 0; x < GRID_W; x++) {
      const int idx = xyToIndex(x, y);
      if (idx >= 0 && idx < NUM_LEDS1) {
        gridFrames[y].push_back(idx);
      }
    }
  }
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

static inline void showStrips() {
  strip1.show();
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

  // Tell the wave engine how many frames we have (grid rows).
  const auto& frames = gridFrames;
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

      // Keep the same parameter meaning as in the original project:
      // nose/tail are in "frame units" (rows here, rings later).
      const float nose = 0.8f + (beatStrength * 2.5f);
      const float tail = 1.5f + (beatStrength * 4.0f);

      const bool reverse = (random(0, 100) < 25);
      addWave(hue, speedCtl, nose, tail, reverse);
    }
  }
#endif

#if ENABLE_FALLBACK_WAVES
  // If no beat is detected for a while, inject a wave so the grid doesn't go idle.
  if ((now - lastBeatMs >= NO_BEAT_FALLBACK_MS) && (now - lastWaveTime >= NO_BEAT_FALLBACK_MS)) {
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

  // IMPORTANT:
  // We keep the strip brightness at 255 and control *all* brightness via frameBrightness.
  // This preserves animation-relative brightness and makes beat pulses very visible.
  strip1.setBrightness(255);

  strip1.clear();
  showStrips();

  resetWaves();

  randomSeed((uint32_t)micros());

  buildGridFrames();

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

  showStrips();
  delay(DELAY_MS);
  yield();
}
