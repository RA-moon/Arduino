#include <Adafruit_NeoPixel.h>
#include <driver/i2s.h>

#define LED_PIN     5    // Pin to which the LED strip is connected
#define NUM_LEDS    60   // Number of LEDs in your strip
#define LED_TYPE    NEO_GRB + NEO_KHZ800  // Type of LED (GRB format and 800 KHz)

// I2S microphone configuration
#define I2S_DATA_IN_PIN  8
#define I2S_BCLK_PIN     9
#define I2S_LRCL_PIN     14


// I2S configuration parameters
void i2s_setup() {
  // I2S configuration: Set the receiver and master mode
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_RX | I2S_MODE_MASTER),  // Combine RX and MASTER mode explicitly
    .sample_rate = 16000,                   // Set sample rate (16kHz for most I2S microphones)
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // 16-bit sample size
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,  // Mono channel (single channel)
    .communication_format = I2S_COMM_FORMAT_I2S,   // I2S protocol
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  // Set interrupt priority
    .dma_buf_count = 8,  // Buffer count
    .dma_buf_len = 1024  // Buffer length
  };

  // Correct the pin configuration for the newer version of ESP32 core
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK_PIN,    // Bit Clock pin
    .ws_io_num = I2S_LRCL_PIN,    // Left-Right Clock pin
    .sd_in_num = I2S_DATA_IN_PIN, // Data In pin (microphone data out)
    .sd_out_num = I2S_PIN_NO_CHANGE  // No output (not needed for input)
  };

  // Install I2S driver with the above configuration
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.show();  // Initialize all pixels to 'off'

  // Set up I2S microphone
  i2s_setup();
}

void loop() {
  int16_t audio_data[1024];
  size_t bytes_read;

  // Read audio data from I2S microphone
  i2s_read(I2S_NUM_0, &audio_data, sizeof(audio_data), &bytes_read, portMAX_DELAY);

  // Process the audio data and calculate BPM
  uint8_t bpm = calculateBPM(audio_data, bytes_read);

  // Map the BPM to LED brightness (e.g., 0-255)
  uint8_t brightness = map(bpm, 60, 180, 0, 255); // Example: Map 60-180 BPM to 0-255 brightness

  // Apply the brightness to the NeoPixels
  setRainbowWaveBrightness(brightness);

  delay(50);  // Add a small delay
}

// Function to calculate BPM based on audio data (very simplified)
uint8_t calculateBPM(int16_t* audio_data, size_t bytes_read) {
  uint16_t max_amplitude = 0;
  for (size_t i = 0; i < bytes_read / 2; i++) {
    int16_t sample = audio_data[i];
    if (abs(sample) > max_amplitude) {
      max_amplitude = abs(sample);
    }
  }

  // Here we're simplifying BPM calculation based on the max amplitude of the signal
  // Typically, you would need more sophisticated algorithms like Fast Fourier Transform (FFT)
  // to detect beats and calculate BPM.
  uint8_t bpm = map(max_amplitude, 0, 5000, 60, 180);  // Simplified mapping
  return bpm;
}

// Function to display rainbow wave with dynamic brightness
void setRainbowWaveBrightness(uint8_t brightness) {
  static uint16_t startIndex = 0;

  for (int i = 0; i < NUM_LEDS; i++) {
    uint16_t hue = (i + startIndex) * 65536L / NUM_LEDS;
    strip.setPixelColor(i, strip.ColorHSV(hue, 255, brightness));
  }

  strip.show();
  startIndex++;  // Shift the rainbow effect
}
