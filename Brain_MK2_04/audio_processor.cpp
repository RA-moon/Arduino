
#include "audio_processor.h"
#include <driver/i2s.h>
#include <arduinoFFT.h>

#define SAMPLE_SIZE 64
#define BEAT_THRESHOLD   3000
#define BEAT_COOLDOWN_MS 200
#define I2S_DATA_IN_PIN  3
#define I2S_BCLK_PIN     2
#define I2S_LRCL_PIN     1

float brightnessPulse = 1.0;
unsigned long lastBeatTime = 0;
double vReal[SAMPLE_SIZE];
double vImag[SAMPLE_SIZE];
ArduinoFFT<double> FFT(vReal, vImag, SAMPLE_SIZE, 16000, false);

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_RX | I2S_MODE_MASTER),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK_PIN,
    .ws_io_num = I2S_LRCL_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_DATA_IN_PIN
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

void processAudio() {
  int32_t buffer[SAMPLE_SIZE];
  size_t bytesRead;
  esp_err_t result = i2s_read(I2S_NUM_0, buffer, SAMPLE_SIZE * sizeof(int32_t), &bytesRead, portMAX_DELAY);
  if (result != ESP_OK || bytesRead == 0) return;

  long energy = 0;
  for (int i = 0; i < SAMPLE_SIZE; i++) {
    int16_t sample = buffer[i] >> 14;
    energy += abs(sample);
    vReal[i] = (double)sample;
    vImag[i] = 0.0;
  }

  FFT.windowing(vReal, SAMPLE_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(vReal, vImag, SAMPLE_SIZE, FFT_FORWARD);
  FFT.complexToMagnitude(vReal, vImag, SAMPLE_SIZE);

  int average = energy / SAMPLE_SIZE;
  unsigned long now = millis();

  if (average > BEAT_THRESHOLD && now - lastBeatTime > BEAT_COOLDOWN_MS) {
    brightnessPulse = 1.0;
    lastBeatTime = now;
  }
}
