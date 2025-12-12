#include <Adafruit_NeoPixel.h>

// ====== USER CONFIGURATION ======
#define PIN        5
#define NUM_LEDS   120
#define BRIGHTNESS 80

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// ---- Animation Frames ----
const uint8_t frameCount = 12;  // Number of frames

// For simplicity, max LEDs in any frame
const uint8_t maxFrameLength = 52;  // Adjust if you add longer frames

// Store frames as 2D array. Each frame begins with the count of LEDs ON in this frame.
const int frames[frameCount][maxFrameLength + 1] = {
  {4,   94, 104, 95, 105},
  {8,   93, 13, 84, 104, 85, 105, 103, 12},
  {20,  33, 93, 34, 103, 15, 14, 17, 18, 84, 104, 85, 105, 35, 10, 105, 36, 11, 12, 104, 115},
  {20,  32, 33, 34, 35, 83, 104, 16, 11, 8, 49, 9, 50, 12, 51, 82, 115, 22, 23, 24, 25},
  {28,  92, 102, 34, 35, 32, 81, 35, 112, 82, 113, 14, 17, 8, 49, 9, 50, 12, 51, 82, 116, 22, 81, 25, 116, 107, 108, 109, 110},
  {32,  44, 91, 45, 101, 31, 81, 36, 112, 30, 37, 6, 48, 17, 37, 7, 49, 18, 38, 8, 50, 11, 51, 22, 26, 13, 87, 106, 116, 54, 55, 56, 57},
  {44,  81, 91, 92, 101, 102, 111, 43, 81, 46, 113, 30, 39, 37, 38, 48, 39, 78, 79, 80, 48, 67, 79, 68, 69, 70, 71, 11, 50, 67, 49, 61, 67, 67, 68, 13, 87, 106, 116, 88, 89, 98, 108, 109, 119},
  {52,  81, 91, 92, 101, 102, 111, 72, 77, 112, 117, 41, 42, 47, 48, 30, 31, 37, 38, 5, 39, 48, 39, 78, 79, 80, 48, 67, 49, 60, 68, 66, 67, 68, 69, 13, 87, 106, 116, 59, 58, 52, 53, 57, 58, 88, 89, 98, 108, 109, 119},
  {36,  80, 81, 90, 91, 100, 101, 77, 76, 77, 78, 40, 79, 40, 78, 30, 39, 37, 38, 60, 69, 61, 70, 62, 71, 67, 68, 13, 87, 106, 116, 88, 89, 98, 108, 109, 119},
  {30,  79, 78, 111, 110, 77, 78, 79, 80, 72, 77, 112, 117, 31, 30, 37, 38, 59, 58, 52, 53, 57, 58, 88, 89, 98, 108, 109, 119},
  {12,  78, 77, 112, 117, 76, 77, 60, 69, 61, 70, 62, 71},
  {4,   80, 39, 89, 119}
};

void showFrame(const int *frame) {
  // Turn all LEDs off first
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0);
  }
  int count = frame[0];
  // Light up specified LEDs (choose any color you want, here is white)
  for (int i = 1; i <= count; i++) {
    if (frame[i] < NUM_LEDS)
      strip.setPixelColor(frame[i], strip.Color(255, 255, 255));
  }
  strip.show();
}

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

void loop() {
  for (uint8_t f = 0; f < frameCount; f++) {
    showFrame(frames[f]);
    delay(500); // Adjust the speed of animation here (ms per frame)
  }
}
