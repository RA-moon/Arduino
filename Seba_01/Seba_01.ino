// 180 NeoPixel rainbow, traveling continuously
// Adafruit_NeoPixel library required

#include <Adafruit_NeoPixel.h>

#define LED_PIN     1      // Pin 1 = TX. Avoid Serial while using this.
#define LED_COUNT   180
#define BRIGHTNESS  100     // 0–255
#define STEP_HUE    256    // hue advance per frame (smaller = slower)
#define DELAY_MS    10     // frame delay

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint16_t baseHue = 0;

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  // Do NOT use Serial on pin 1 here.
}

void loop() {
  // Fill strip with a rainbow whose phase moves by baseHue
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    // Map pixel index to hue across full circle
    uint16_t hue = (uint32_t)i * 65535 / LED_COUNT;
    uint32_t c = strip.ColorHSV(hue + baseHue, 255, 255); // full sat, full value
    strip.setPixelColor(i, strip.gamma32(c));              // gamma-correct
  }
  strip.show();

  baseHue += STEP_HUE;          // move the rainbow
  delay(DELAY_MS);
}
