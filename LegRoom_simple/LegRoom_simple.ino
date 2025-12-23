#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN         17
#define NUM_LEDS        1440
#define LED_FPS         15          // ~43ms per show() => keep FPS low/stable
#define BRIGHTNESS      80          // 0..255 (keep low for power)

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

static uint8_t hueBase = 0;
static uint32_t lastFrameMs = 0;
static const uint32_t frameDelayMs = 1000 / LED_FPS;

static inline void renderRainbow(uint8_t baseHue8) {
  uint16_t baseHue16 = (uint16_t)baseHue8 << 8;   // 0..65535
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // “delta=1” look (repeats every 256 pixels)
    uint16_t hue16 = baseHue16 + (uint16_t)((uint32_t)i * 256U);
    strip.setPixelColor(i, strip.ColorHSV(hue16));
  }
}

void setup() {
  delay(300);
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();
  lastFrameMs = millis();
}

void loop() {
  uint32_t now = millis();
  if (now - lastFrameMs >= frameDelayMs) {
    lastFrameMs += frameDelayMs;   // steadier cadence
    renderRainbow(hueBase);
    hueBase += 4;                  // speed
    strip.show();
    yield();
  }
}
