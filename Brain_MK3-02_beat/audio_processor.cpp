#include "audio_processor.h"
#include <Arduino.h>

// Arduino-ESP32 I2S API (core 3.x)
#include <ESP_I2S.h>

// FFT library (bundled into this sketch folder)
#include "arduinoFFT.h"

// === Compile-time switches ===
#ifndef AUDIO_ENABLE_I2S
#define AUDIO_ENABLE_I2S 1
#endif

// === I2S pin mapping (adjust to your wiring) ===
// Recommended: keep to "safe" GPIOs on your board and avoid pins already used for LEDs.
#ifndef I2S_BCLK_PIN
#define I2S_BCLK_PIN 21
#endif
#ifndef I2S_WS_PIN
#define I2S_WS_PIN 22
#endif
#ifndef I2S_DIN_PIN
#define I2S_DIN_PIN 23
#endif
#ifndef I2S_MCLK_PIN
#define I2S_MCLK_PIN -1
#endif

// === SPH0645 settings ===
// The SPH0645 drives DATA on either the left slot (WS=0) or right slot (WS=1),
// depending on SEL. SEL=LOW -> left slot, SEL=HIGH -> right slot.
#ifndef SPH0645_CHANNEL
#define SPH0645_CHANNEL 0 // 0=left, 1=right
#endif

// SPH0645 outputs 24-bit samples in a 32-bit I2S slot (8 padding bits). Shifting by 8
// usually yields a signed 24-bit value in a 32-bit container.
#ifndef SPH0645_RAW_SHIFT
#define SPH0645_RAW_SHIFT 8
#endif

// === FFT / beat parameters ===
static constexpr uint32_t kSampleRateHz = 32000;   // SPH0645 datasheet is 32k..64k (BCLK = 64*Fs)
static constexpr uint16_t kFftSamples   = 512;     // power of 2

static constexpr float kBassMinHz = 40.0f;
static constexpr float kBassMaxHz = 180.0f;

static constexpr float kEmaAlpha       = 0.08f;    // smoothing for bass energy
static constexpr float kBeatThreshold  = 1.8f;     // energy must exceed (ema * threshold)
static constexpr float kBeatRiseFactor = 0.10f;    // require a minimum rise vs previous energy
static constexpr uint32_t kMinBeatIntervalMs = 120;

// Baseline pulse multiplier (the main loop further decays + clamps it).
float brightnessPulse = 1.0f;

static inline float clamp01(float x) {
  if (x < 0.0f) return 0.0f;
  if (x > 1.0f) return 1.0f;
  return x;
}

// Beat event (edge triggered)
static volatile bool  s_beatPending  = false;
static volatile float s_beatStrength = 0.0f;

bool consumeBeat(float* strength) {
  if (!s_beatPending) return false;
  s_beatPending = false;
  if (strength) *strength = s_beatStrength;
  return true;
}

static I2SClass I2S;
static bool s_i2sOk = false;

// Interleaved stereo (L,R) 32-bit words
static int32_t s_i2sRaw[kFftSamples * 2];

static double s_vReal[kFftSamples];
static double s_vImag[kFftSamples];
static ArduinoFFT<double> s_fft = ArduinoFFT<double>(s_vReal, s_vImag, kFftSamples, kSampleRateHz);

static float s_bassEma = 0.0f;
static float s_prevBass = 0.0f;
static uint32_t s_lastBeatMs = 0;

static void fakeAudioPulse() {
  static unsigned long lastKickMs = 0;
  const unsigned long now = millis();

  if (now - lastKickMs > 120UL && (uint8_t)random(0, 100) < 6) {
    brightnessPulse = 1.6f;
    s_beatPending = true;
    s_beatStrength = 0.7f;
    lastKickMs = now;
  }

  if (brightnessPulse < 1.0f) brightnessPulse = 1.0f;
}

void setupI2S() {
#if AUDIO_ENABLE_I2S
  // STD mode uses separate DIN/DOUT pins; we only need DIN for a microphone.
  // setPins(bclk, ws, dout, din, mclk)
  I2S.setPins(I2S_BCLK_PIN, I2S_WS_PIN, -1, I2S_DIN_PIN, I2S_MCLK_PIN);

  // Use 32-bit stereo so BCLK = 64 * Fs (required by SPH0645).
  s_i2sOk = I2S.begin(I2S_MODE_STD, kSampleRateHz, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO);

  if (!s_i2sOk) {
    // Fall back to fake pulses so the project still runs.
    Serial.println("I2S init failed -> using fake audio pulse");
  }
#else
  s_i2sOk = false;
#endif
}

void processAudio() {
#if AUDIO_ENABLE_I2S
  if (!s_i2sOk) {
    fakeAudioPulse();
    return;
  }

  const size_t wantBytes = sizeof(s_i2sRaw);
  const size_t gotBytes = I2S.readBytes((char*)s_i2sRaw, wantBytes);
  if (gotBytes < wantBytes) {
    // Not enough data yet.
    return;
  }

  // Convert raw I2S words into a mono buffer (choose left or right slot).
  double mean = 0.0;
  for (uint16_t i = 0; i < kFftSamples; i++) {
    const int32_t w = s_i2sRaw[(i * 2) + (SPH0645_CHANNEL ? 1 : 0)];
    const int32_t s = (SPH0645_RAW_SHIFT > 0) ? (w >> SPH0645_RAW_SHIFT) : w;
    s_vReal[i] = (double)s;
    s_vImag[i] = 0.0;
    mean += s_vReal[i];
  }

  // DC removal (simple mean subtraction).
  mean /= (double)kFftSamples;
  for (uint16_t i = 0; i < kFftSamples; i++) {
    s_vReal[i] -= mean;
  }

  // FFT
  s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  s_fft.compute(FFTDirection::Forward);
  s_fft.complexToMagnitude();

  // Bass energy from magnitude bins
  const uint16_t maxBin = (kFftSamples >> 1) - 1;
  uint16_t binMin = (uint16_t)((kBassMinHz * (float)kFftSamples) / (float)kSampleRateHz);
  uint16_t binMax = (uint16_t)((kBassMaxHz * (float)kFftSamples) / (float)kSampleRateHz);
  if (binMin < 1) binMin = 1;
  if (binMax > maxBin) binMax = maxBin;

  float bass = 0.0f;
  for (uint16_t b = binMin; b <= binMax; b++) {
    const float m = (float)s_vReal[b];
    bass += m;
  }

  // Smooth baseline
  if (s_bassEma <= 0.0001f) s_bassEma = bass;
  s_bassEma = (1.0f - kEmaAlpha) * s_bassEma + kEmaAlpha * bass;

  // Beat decision
  const uint32_t now = millis();
  const float rise = bass - s_prevBass;
  const bool intervalOk = (now - s_lastBeatMs) >= kMinBeatIntervalMs;
  const bool above = bass > (s_bassEma * kBeatThreshold);
  const bool rising = rise > (s_bassEma * kBeatRiseFactor);

  if (intervalOk && above && rising) {
    const float ratio = bass / (s_bassEma + 1e-3f);
    const float strength = clamp01((ratio - kBeatThreshold) / kBeatThreshold);

    s_beatPending = true;
    s_beatStrength = strength;
    s_lastBeatMs = now;

    // Also drive the global brightness pulse.
    const float pulse = 1.0f + (0.9f * strength);
    if (brightnessPulse < pulse) brightnessPulse = pulse;
  }

  s_prevBass = bass;

  if (brightnessPulse < 1.0f) brightnessPulse = 1.0f;
#else
  fakeAudioPulse();
#endif
}
