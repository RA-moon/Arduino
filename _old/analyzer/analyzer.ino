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


Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, LED_TYPE);


int brightness = 127;
unsigned long lastBeatTime = 0;
unsigned long lastBPMReportTime = 0;
int currentBPM = 0;


/*
These values can be changed in order to evaluate the functions
*/
const uint16_t samples = 64; //This value MUST ALWAYS be a power of 2
const double signalFrequency = 1000;
const double samplingFrequency = 5000;
const uint8_t amplitude = 100;

/*
These are the input and output vectors
Input vectors receive computed results from FFT
*/
double vReal[samples];
double vImag[samples];


// FFT buffers
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLE_SIZE, 16000, false);

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
  updateSpectrum();
}

void updateSpectrum() {
  int32_t buffer[SAMPLE_SIZE];
  size_t bytesRead;
  esp_err_t result = i2s_read(I2S_NUM_0, &buffer, SAMPLE_SIZE * sizeof(int32_t), &bytesRead, portMAX_DELAY);

  if (result != ESP_OK || bytesRead == 0) return;

  for (int i = 0; i < SAMPLE_SIZE; i++) {
    vReal[i] = (double)(buffer[i] >> 14); // scale down
    vImag[i] = 0;
  }

  FFT.windowing(vReal, SAMPLE_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(vReal, vImag, SAMPLE_SIZE, FFT_FORWARD);
  FFT.complexToMagnitude(vReal, vImag, SAMPLE_SIZE);

  strip.clear();
  for (int band = 0; band < MATRIX_WIDTH; band++) {
    int index = band + 1;  // skip DC bin
    double magnitude = vReal[index];
    int height = map(magnitude, 0, 1000, 0, MATRIX_HEIGHT);
    height = constrain(height, 0, MATRIX_HEIGHT);
    drawColumn(band, height);
  }
  strip.show();
}

void drawColumn(int col, int height) {
  for (int row = 0; row < height; row++) {
    int ledIndex = getLedIndex(col, row);
    strip.setPixelColor(ledIndex, strip.ColorHSV(row * 4000));
  }
}

int getLedIndex(int x, int y) {
  if (y % 2 == 0) {
    return y * MATRIX_WIDTH + x;  // even row: left to right
  } else {
    return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);  // odd row: right to left (zigzag)
  }
}
