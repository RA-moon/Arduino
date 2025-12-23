#include <FastLED.h>
#include <arduinoFFT.h>
#include <driver/i2s.h>

/////////////////////
// LED Config
/////////////////////

#define LED_PIN         20
#define NUM_LEDS        1440
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define BASE_BRIGHTNESS 100
#define MAX_BRIGHTNESS  200
#define LED_FPS         60

CRGB leds[NUM_LEDS];

/////////////////////
// Brightness Control
/////////////////////

float currentBrightness = BASE_BRIGHTNESS;
float targetBrightness = BASE_BRIGHTNESS;
float fadeSpeed = 0.05;

/////////////////////
// Timing
/////////////////////

unsigned long lastUpdate = 0;
const int frameDelay = 1000 / LED_FPS;

/////////////////////
// FFT Config
/////////////////////

#define SAMPLES         64
#define SAMPLING_FREQ   16000.0

double vReal[SAMPLES];
double vImag[SAMPLES];

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQ);

double beatThreshold = 2.0; // Relative threshold
bool beatDetected = false;

/////////////////////
// Color Animation
/////////////////////

uint8_t hueBase = 0;

/////////////////////
// I2S Config for SPH0645
/////////////////////

#define I2S_WS      7
#define I2S_SD      5
#define I2S_SCK     6

i2s_config_t i2s_config = {
  .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
  .sample_rate = (int)SAMPLING_FREQ,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = I2S_COMM_FORMAT_I2S,
  .intr_alloc_flags = 0,
  .dma_buf_count = 4,
  .dma_buf_len = 256,
  .use_apll = false,
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0
};

i2s_pin_config_t pin_config = {
  .bck_io_num = I2S_SCK,
  .ws_io_num = I2S_WS,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = I2S_SD
};

/////////////////////
// BPM Calculation
/////////////////////

unsigned long lastBpmPrint = 0;
unsigned long bpmWindowStart = 0;
int beatCount = 0;

/////////////////////
// Setup
/////////////////////

void setup() {
  delay(1000);
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BASE_BRIGHTNESS);

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);

  bpmWindowStart = millis();
  lastBpmPrint = millis();
}

/////////////////////
// Beat Detection
/////////////////////

void detectBeat() {
  int32_t samples[SAMPLES];
  size_t bytesRead;

  i2s_read(I2S_NUM_0, (char*)samples, sizeof(samples), &bytesRead, portMAX_DELAY);

  if (bytesRead < sizeof(samples)) {
    Serial.println("I2S read underrun");
    return;
  }

  for (int i = 0; i < SAMPLES; i++) {
    // Normalize 24-bit data to float in [-1.0, 1.0]
    vReal[i] = (float)samples[i] / 8388608.0; // 2^23
    vImag[i] = 0;
  }

  // Remove DC offset
  double mean = 0;
  for (int i = 0; i < SAMPLES; i++) mean += vReal[i];
  mean /= SAMPLES;
  for (int i = 0; i < SAMPLES; i++) vReal[i] -= mean;

  // Perform FFT
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();

  // Sum bass bins (bins 1–4)
  double bassSum = 0;
  for (int i = 1; i <= 4; i++) {
    bassSum += vReal[i];
  }

  static double bassAvg = 0;
  bassAvg = bassAvg * 0.9 + bassSum * 0.1;

  if (bassSum > bassAvg * beatThreshold) {
    beatDetected = true;
    targetBrightness = MAX_BRIGHTNESS;
    beatCount++;
    Serial.println("Beat!");
  }
}

/////////////////////
// Main Loop
/////////////////////

void loop() {
  unsigned long now = millis();

  // Update LEDs at consistent frame rate
  if (now - lastUpdate >= frameDelay) {
    lastUpdate = now;

    fill_rainbow(leds, NUM_LEDS, hueBase, 1);  // Narrow rainbow pattern
    hueBase += 4;  // Fast scroll

    if (beatDetected) {
      currentBrightness = targetBrightness;
      beatDetected = false;
    } else {
      currentBrightness = currentBrightness * (1.0 - fadeSpeed) + BASE_BRIGHTNESS * fadeSpeed;
    }

    FastLED.setBrightness((uint8_t)constrain(currentBrightness, 0, 255));
    FastLED.show();
  }

  // Run beat detection every ~20ms
  static unsigned long lastBeatCheck = 0;
  if (now - lastBeatCheck >= 20) {
    lastBeatCheck = now;
    detectBeat();
  }

  // Print BPM every 5 seconds
  if (now - lastBpmPrint >= 5000) {
    float minutes = (now - bpmWindowStart) / 60000.0;
    float bpm = (minutes > 0) ? beatCount / minutes : 0;

    Serial.print("BPM: ");
    Serial.println((int)bpm);

    lastBpmPrint = now;
    bpmWindowStart = now;
    beatCount = 0;
  }
}
