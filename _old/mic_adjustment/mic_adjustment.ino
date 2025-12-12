#include <Adafruit_NeoPixel.h>
#include <driver/i2s.h>

#define LED_PIN     5
#define NUM_LEDS    60
#define LED_TYPE    NEO_GRB + NEO_KHZ800

#define I2S_DATA_IN_PIN  3
#define I2S_BCLK_PIN     2
#define I2S_LRCL_PIN     1

#define BEAT_THRESHOLD   3000
#define BEAT_COOLDOWN_MS 200

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, LED_TYPE);

int brightness = 127;  // Start at 50% brightness
unsigned long lastBeatTime = 0;
unsigned long lastBPMReportTime = 0;
int currentBPM = 0;

// I2S configuration
void i2s_setup() {
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

void setup() {
  Serial.begin(115200);
  Serial.flush();
  delay(2000);
  strip.begin();
  strip.show();
  i2s_setup();
  Serial.println("Setup complete. Starting...");
}

void loop() {
  detectBeat();
  rainbowWave();
}

void detectBeat() {
  const int samples = 64;
  int32_t buffer[samples];
  size_t bytesRead;
  esp_err_t result = i2s_read(I2S_NUM_0, &buffer, samples * sizeof(int32_t), &bytesRead, portMAX_DELAY);

  if (result == ESP_OK && bytesRead > 0) {
    long sum = 0;
    for (int i = 0; i < samples; i++) {
      sum += abs(buffer[i] >> 14);  // Scale down the 24-bit data from SPH0645
    }
    int average = sum / samples;

    unsigned long now = millis();
    if (average > BEAT_THRESHOLD && now - lastBeatTime > BEAT_COOLDOWN_MS) {
      brightness = 255;
      int interval = now - lastBeatTime;
      int bpm = 60000 / interval;
      if (bpm != currentBPM) {
        currentBPM = bpm;
        Serial.print("BPM: ");
        Serial.println(currentBPM);
      }
      lastBeatTime = now;
    }
  }
}

void rainbowWave() {
  static uint16_t startIndex = 0;

  if (brightness > 127) {  // Min brightness = 50%
    strip.setBrightness(brightness);
    brightness -= 10;
    if (brightness < 127) brightness = 127;
  } else {
    strip.setBrightness(127);
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    uint16_t hue = (i + startIndex) * 65536L / NUM_LEDS;
    strip.setPixelColor(i, strip.ColorHSV(hue));
  }
  strip.show();
  delay(20);
  startIndex++;
}
