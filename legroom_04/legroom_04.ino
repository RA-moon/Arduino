#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <arduinoFFT.h>
#include <driver/i2s.h>

/////////////////////
// LED Config (1440 rainbow like before)
/////////////////////

#define LED_PIN         20          // external strip DATA pin (avoid strap pins 4/5/8/9/15)
#define NUM_LEDS        1440
#define BASE_BRIGHTNESS 100
#define MAX_BRIGHTNESS  200

// 1440x WS2812 needs ~43ms per show() -> 60 FPS is impossible; use ~12–18 for stability
#define LED_FPS         15

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

float currentBrightness = BASE_BRIGHTNESS;
float fadeSpeed = 0.05f;

unsigned long lastUpdate = 0;
const int frameDelay = 1000 / LED_FPS;

uint8_t hueBase = 0;

/////////////////////
// FFT Config
/////////////////////

#define SAMPLES         256
#define SAMPLING_FREQ   16000.0

double vReal[SAMPLES];
double vImag[SAMPLES];

ArduinoFFT<double> FFT(vReal, vImag, SAMPLES, SAMPLING_FREQ);

double beatThreshold = 2.0;
bool beatDetected = false;

/////////////////////
// I2S Config for SPH0645
/////////////////////

#define I2S_WS      7
#define I2S_SD      5      // NOTE: ESP32-C6 strap pin; if boot issues, move to e.g. GPIO3
#define I2S_SCK     6

// SPH0645: often needs shifting; try 8 or 14 if levels look wrong
#define MIC_SHIFT   14
#define MIC_NORM    131072.0  // 2^17

/////////////////////
// BPM Calculation
/////////////////////

unsigned long lastBpmPrint = 0;
unsigned long bpmWindowStart = 0;
int beatCount = 0;

/////////////////////
// Helpers
/////////////////////

static inline void renderRainbow(uint8_t baseHue8) {
  uint16_t baseHue16 = ((uint16_t)baseHue8) << 8; // 0..65535
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // delta=1 like FastLED fill_rainbow(..., delta=1)
    uint16_t hue16 = baseHue16 + (uint16_t)(i * 256U);
    strip.setPixelColor(i, strip.ColorHSV(hue16));
  }
}

static void setupI2S() {
  i2s_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = (int)SAMPLING_FREQ;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;  // try ONLY_RIGHT if SEL differs
#if defined(I2S_COMM_FORMAT_STAND_I2S)
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
#else
  cfg.communication_format = I2S_COMM_FORMAT_I2S;
#endif
  cfg.intr_alloc_flags = 0;
  cfg.dma_buf_count = 6;      // a bit larger to survive LED show() blocking
  cfg.dma_buf_len = SAMPLES;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = false;
  cfg.fixed_mclk = 0;

  i2s_pin_config_t pins;
  memset(&pins, 0, sizeof(pins));
  pins.bck_io_num = I2S_SCK;
  pins.ws_io_num = I2S_WS;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = I2S_SD;

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

/////////////////////
// Beat Detection
/////////////////////

void detectBeat() {
  static int32_t samples[SAMPLES];
  size_t bytesRead = 0;

  esp_err_t err = i2s_read(I2S_NUM_0, (void*)samples, sizeof(samples), &bytesRead, pdMS_TO_TICKS(50));
  if (err != ESP_OK || bytesRead < sizeof(samples)) return;

  for (int i = 0; i < SAMPLES; i++) {
    int32_t s = samples[i];
    s >>= MIC_SHIFT;
    vReal[i] = (double)s / MIC_NORM;
    vImag[i] = 0.0;
  }

  // Remove DC offset
  double mean = 0.0;
  for (int i = 0; i < SAMPLES; i++) mean += vReal[i];
  mean /= (double)SAMPLES;
  for (int i = 0; i < SAMPLES; i++) vReal[i] -= mean;

  // FFT
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();

  // Bass bins: with 256 samples @16k -> bin width 62.5 Hz => bins 1..3 ~ 62..187 Hz
  double bassSum = 0.0;
  for (int i = 1; i <= 3; i++) bassSum += vReal[i];

  static double bassAvg = 0.0;
  bassAvg = bassAvg * 0.90 + bassSum * 0.10;

  static unsigned long lastBeatMs = 0;
  unsigned long now = millis();

  if (bassAvg > 1e-9 && bassSum > bassAvg * beatThreshold) {
    if (now - lastBeatMs > 200) { // cooldown
      beatDetected = true;
      beatCount++;
      lastBeatMs = now;
    }
  }
}

/////////////////////
// Setup
/////////////////////

void setup() {
  delay(500);
  Serial.begin(115200);

  strip.begin();
  strip.setBrightness(BASE_BRIGHTNESS);
  strip.clear();
  strip.show();

  setupI2S();

  bpmWindowStart = millis();
  lastBpmPrint = millis();
}

/////////////////////
// Main Loop
/////////////////////

void loop() {
  unsigned long now = millis();

  // Run beat detection ~ every 20ms (may block briefly waiting for samples)
  static unsigned long lastBeatCheck = 0;
  if (now - lastBeatCheck >= 20) {
    lastBeatCheck = now;
    detectBeat();
  }

  // LED update at chosen FPS
  if (now - lastUpdate >= (unsigned long)frameDelay) {
    lastUpdate = now;

    renderRainbow(hueBase);
    hueBase += 4; // fast scroll like before

    if (beatDetected) {
      currentBrightness = MAX_BRIGHTNESS;
      beatDetected = false;
    } else {
      currentBrightness = currentBrightness * (1.0f - fadeSpeed) + (float)BASE_BRIGHTNESS * fadeSpeed;
    }

    strip.setBrightness((uint8_t)constrain((int)currentBrightness, 0, 255));
    strip.show();
  }

  // Print BPM every 5 seconds
  if (now - lastBpmPrint >= 5000) {
    float minutes = (now - bpmWindowStart) / 60000.0f;
    float bpm = (minutes > 0.0f) ? (beatCount / minutes) : 0.0f;

    Serial.print("BPM: ");
    Serial.println((int)bpm);

    lastBpmPrint = now;
    bpmWindowStart = now;
    beatCount = 0;
  }
}
