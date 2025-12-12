#include <FastLED.h>

// === CONFIGURATION ===
#define DATA_PIN      16
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB
#define NUM_LEDS      1440       // Total number of LEDs

#define BRIGHTNESS    120
#define SPEED_RAINBOW 10         // Rainbow speed
#define SPEED_FADE    5          // Fade in/out speed

CRGB leds[NUM_LEDS];

// State variables
uint8_t hueOffset = 0;
int fadeBrightness = 0;
int fadeDirection = 1;   // 1 = fade in, -1 = fade out

// Color transition timing
unsigned long colorCycleStart = 0;
const unsigned long COLOR_CYCLE_DURATION = 1800000UL;  // 30 minutes in milliseconds

void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  colorCycleStart = millis();
}

void loop() {
  // === Clear strip ===
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // === Rainbow: regions 0–32 + 40–43 ===
  const int activeRainbowLeds = 33 + 4;  // 37
  int rainbowIndex = 0;

  // Region 0–32
  for (int i = 0; i <= 32; i++) {
    uint8_t hue = hueOffset + (rainbowIndex * 255 / activeRainbowLeds);
    leds[i] = CHSV(hue, 255, 255);
    rainbowIndex++;
  }

  // Region 40–43
  for (int i = 40; i <= 43; i++) {
    uint8_t hue = hueOffset + (rainbowIndex * 255 / activeRainbowLeds);
    leds[i] = CHSV(hue, 255, 255);
    rainbowIndex++;
  }

  // === Fade + Flicker + Very Slow Color Blend: region 33–39 ===

  // Calculate how far along we are in the color cycle (0.0 to 1.0 to 0.0)
  unsigned long elapsed = (millis() - colorCycleStart) % COLOR_CYCLE_DURATION;
  float phase = (float)elapsed / (COLOR_CYCLE_DURATION / 2);  // 0 to 2

  // Triangle wave for smooth back-and-forth
  float interpFactor = phase <= 1.0 ? phase : 2.0 - phase;

  // Interpolate hue from green (96) to blue (160)
  uint8_t startHue = 96;
  uint8_t endHue = 160;
  uint8_t interpHue = startHue + (endHue - startHue) * interpFactor;

  // Add flicker only in the mid-brightness range
  int effectiveBrightness = fadeBrightness;
  if (fadeBrightness > 80 && fadeBrightness < 180) {
    effectiveBrightness += random(-30, 30);  // Mild flicker
    effectiveBrightness = constrain(effectiveBrightness, 0, 255);
  }

  CRGB fadeColor = CHSV(interpHue, 255, effectiveBrightness);

  for (int i = 33; i <= 39; i++) {
    leds[i] = fadeColor;
  }

  FastLED.show();

  // === Update animation state ===
  hueOffset += SPEED_RAINBOW;

  fadeBrightness += fadeDirection * SPEED_FADE;
  if (fadeBrightness >= 255) {
    fadeBrightness = 255;
    fadeDirection = -1;
  } else if (fadeBrightness <= 0) {
    fadeBrightness = 0;
    fadeDirection = 1;
  }

  delay(30);
}
