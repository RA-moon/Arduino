#include <Arduino.h>
#include <FastLED.h>
#include "circle_expansion_frames.h"

// LED Matrix settings
#define MATRIX_WIDTH 18
#define MATRIX_HEIGHT 16
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define DATA_PIN 5

CRGB leds[NUM_LEDS];

// Helper: map 2D matrix coords to 1D index (zigzag layout)
int getIndex(int x, int y) {
  if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return -1;
  if (y % 2 == 0) {
    return y * MATRIX_WIDTH + x;
  } else {
    return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
  }
}

// Set LED at (x, y) with given color
void setLed(int x, int y, CRGB color) {
  int i = getIndex(x, y);
  if (i >= 0 && i < NUM_LEDS) {
    leds[i] = color;
  }
}

// Clear all LEDs
void clearMatrix() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

// Push the LED changes to the hardware
void showMatrix() {
  FastLED.show();
}

// Draw a ring frame using a list of (x, y) coordinates
void drawFrame(const vector<pair<int, int>>& frame, CRGB color) {
  clearMatrix();
  for (auto& coord : frame) {
    setLed(coord.first, coord.second, color);
  }
  showMatrix();
}

// Animate all ring frames
void animateRings() {
  const int delayTime = 100;
  CRGB color = CRGB::Blue;

  drawFrame(frame_1, color); delay(delayTime);
  drawFrame(frame_2, color); delay(delayTime);
  drawFrame(frame_3, color); delay(delayTime);
  drawFrame(frame_4, color); delay(delayTime);
  drawFrame(frame_5, color); delay(delayTime);
  drawFrame(frame_6, color); delay(delayTime);
  drawFrame(frame_7, color); delay(delayTime);
  drawFrame(frame_8, color); delay(delayTime);
  drawFrame(frame_9, color); delay(delayTime);
  drawFrame(frame_10, color); delay(delayTime);
  drawFrame(frame_11, color); delay(delayTime);
  drawFrame(frame_12, color); delay(delayTime);
  drawFrame(frame_13, color); delay(delayTime);
}

void setup() {
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
}

void loop() {
  animateRings();
  delay(1000);
}
