#include <Adafruit_NeoPixel.h>
#include "animtated_circles.h"

// ====== CONFIGURATION ======
#define PIN              5
#define NUM_LEDS         120
#define BRIGHTNESS       80
#define INTERP_STEPS     10   // Steps per wave fade
#define FRAMES_PER_WAVE  5
#define MAX_WAVES        5
#define WAVE_DELAY_STEPS 6    // Steps between new wave launches

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// Fade profile for bell curve wave shape
float fadeProfile[FRAMES_PER_WAVE] = {0.2, 0.5, 1.0, 0.5, 0.2};

struct Wave {
  int startFrame;
  int step;
  uint32_t color;
  bool active;
};

Wave waves[MAX_WAVES];
int frameStepCounter = 0;
int waveColorIndex = 0;
int nextWaveStartFrame = 0;  // Ensures flowing frame order

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

uint32_t blendColors(uint32_t c1, uint32_t c2) {
  uint8_t r = min((uint8_t)(c1 >> 16) + (uint8_t)(c2 >> 16), 255);
  uint8_t g = min((uint8_t)(c1 >> 8 & 0xFF) + (uint8_t)(c2 >> 8 & 0xFF), 255);
  uint8_t b = min((uint8_t)(c1 & 0xFF) + (uint8_t)(c2 & 0xFF), 255);
  return strip.Color(r, g, b);
}

void loop() {
  static std::vector<std::vector<int>> frames = getAnimationFrames();
  int totalFrames = frames.size();

  // Clear LED buffer
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0);
  }

  // Update all active waves
  for (int w = 0; w < MAX_WAVES; w++) {
    Wave &wave = waves[w];
    if (!wave.active) continue;

    float fadeStep = float(wave.step) / INTERP_STEPS;

    for (int i = 0; i < FRAMES_PER_WAVE; i++) {
      int frameIndex = (wave.startFrame + i) % totalFrames;
      float frameFade = fadeProfile[i] * (1.0 - fadeStep);
      uint8_t val = BRIGHTNESS * frameFade;
      uint32_t waveColor = strip.ColorHSV(wave.color, 255, val);

      for (int led : frames[frameIndex]) {
        if (led >= 0 && led < NUM_LEDS) {
          uint32_t prev = strip.getPixelColor(led);
          strip.setPixelColor(led, blendColors(prev, waveColor));
        }
      }
    }

    wave.step++;
    if (wave.step > INTERP_STEPS) wave.active = false;
  }

  // Periodically launch new wave
  if (frameStepCounter++ >= WAVE_DELAY_STEPS) {
    frameStepCounter = 0;

    for (int w = 0; w < MAX_WAVES; w++) {
      if (!waves[w].active) {
        waves[w].startFrame = nextWaveStartFrame;
        nextWaveStartFrame = (nextWaveStartFrame + 1) % totalFrames;

        waves[w].step = 0;
        waves[w].color = (waveColorIndex * 65536L / 50) % 65536;
        waves[w].active = true;

        waveColorIndex++;
        break;
      }
    }
  }

  strip.show();
  delay(30);
}
