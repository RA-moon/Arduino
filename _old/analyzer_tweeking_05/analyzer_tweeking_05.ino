#include <Adafruit_NeoPixel.h>
#include <driver/i2s.h>
#include <arduinoFFT.h>

#define LED_PIN     5
#define MATRIX_WIDTH  16
#define MATRIX_HEIGHT 16
#define NUM_LEDS    (MATRIX_WIDTH * MATRIX_HEIGHT)
#define LED_TYPE    NEO_GRB + NEO_KHZ800

#define I2S_DATA_IN_PIN  3
#define I2S_BCLK_PIN     2
#define I2S_LRCL_PIN     1

#define SAMPLE_SIZE 64
#define BEAT_THRESHOLD   3000
#define BEAT_COOLDOWN_MS 200
#define BPM_REPORT_INTERVAL_MS 10000

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, LED_TYPE);

int brightness = 127;
unsigned long lastBeatTime = 0;
unsigned long lastBPMReportTime = 0;
int beatCount = 0;

unsigned long bpmWindowStart = 0;

volatile unsigned long beatTimestamps[128];
volatile int beatIndex = 0;

double vReal[SAMPLE_SIZE];
double vImag[SAMPLE_SIZE];

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLE_SIZE, 16000, false);

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  i2s_pin_config_t pin_config;
  pin_config.bck_io_num = I2S_BCLK_PIN;
  pin_config.ws_io_num = I2S_LRCL_PIN;
  pin_config.data_out_num = I2S_PIN_NO_CHANGE;
  pin_config.data_in_num = I2S_DATA_IN_PIN;

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.setBrightness(brightness);
  strip.show();
  setupI2S();
  bpmWindowStart = millis();
}

void loop() {
  processAudio();
  reportBPM();
}

void processAudio() {
  int16_t samples_in[SAMPLE_SIZE];
  size_t bytesRead;

  i2s_read(I2S_NUM_0, (void*)samples_in, sizeof(samples_in), &bytesRead, portMAX_DELAY);

  for (int i = 0; i < SAMPLE_SIZE; i++) {
    vReal[i] = (double)samples_in[i];
    vImag[i] = 0.0;
  }

  FFT.compute(FFT_FORWARD);

  double sum = 0;
  for (int i = 2; i < 15; i++) {
    sum += vReal[i];
  }

  unsigned long currentTime = millis();

  if (sum > BEAT_THRESHOLD && currentTime - lastBeatTime > BEAT_COOLDOWN_MS) {
    lastBeatTime = currentTime;
    beatTimestamps[beatIndex++ % 128] = currentTime;
    triggerBeatEffect();
  }
}

void reportBPM() {
  unsigned long currentTime = millis();
  if (currentTime - lastBPMReportTime >= BPM_REPORT_INTERVAL_MS) {
    lastBPMReportTime = currentTime;

    int beats = 0;
    for (int i = 0; i < 128; i++) {
      if (beatTimestamps[i] > 0 && currentTime - beatTimestamps[i] <= BPM_REPORT_INTERVAL_MS) {
        beats++;
      }
    }

    int bpm = (beats * 60000) / BPM_REPORT_INTERVAL_MS;
    Serial.print("Average BPM (last 10s): ");
    Serial.println(bpm);
  }
}

void triggerBeatEffect() {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(random(100,255), random(100,255), random(100,255)));
  }
  strip.show();
}
