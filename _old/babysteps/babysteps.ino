#include <Adafruit_NeoPixel.h>
#include <driver/i2s.h>

#define LED_PIN     5    // Pin to which the LED strip is connected
#define NUM_LEDS    60   // Number of LEDs in your strip
#define LED_TYPE    NEO_GRB + NEO_KHZ800  // Type of LED (GRB format and 800 KHz)

// I2S microphone configuration
#define I2S_DATA_IN_PIN  8
#define I2S_BCLK_PIN     9
#define I2S_LRCL_PIN     14


Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, LED_TYPE);

void setup() {
  strip.begin();
  strip.show();  // Initialize all pixels to 'off'
}

void loop() {
  rainbowWave();
}

void rainbowWave() {
  static uint16_t startIndex = 0;

  // Generate rainbow colors across the strip
  for (int i = 0; i < NUM_LEDS; i++) {
    uint16_t hue = (i + startIndex) * 65536L / NUM_LEDS;
    strip.setPixelColor(i, strip.ColorHSV(hue));  // HSV color model
  }

  // Display the rainbow wave effect
  strip.show();
  delay(20);  // Delay to create wave effect speed

  startIndex++;  // Shift the rainbow across the strip
}