#include <Adafruit_NeoPixel.h>

// ====== USER CONFIGURATION ======
#define PIN        5          // LED data pin
#define NUM_LEDS   120        // Set this to the total number of LEDs
#define BRIGHTNESS 80         // Max 255

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// ====== YOUR FRAMES (from previous step) ======
std::vector<std::vector<int>> frames = {
    {94, 104, 95, 105},
    {93, 13, 84, 104, 85, 105, 103, 12},
    {33, 93, 34, 103, 15, 14, 17, 18, 84, 104, 85, 105, 35, 10, 105, 36, 11, 12, 104, 115},
    {32, 33, 34, 35, 83, 104, 16, 11, 8, 49, 9, 50, 12, 51, 82, 115, 22, 23, 24, 25},
    {92, 102, 34, 35, 32, 81, 35, 112, 82, 113, 14, 17, 8, 49, 9, 50, 12, 51, 82, 116, 22, 81, 25, 116, 107, 108, 109, 110},
    {44, 91, 45, 101, 31, 81, 36, 112, 30, 37, 6, 48, 17, 37, 7, 49, 18, 38, 8, 50, 11, 51, 22, 26, 13, 87, 106, 116, 54, 55, 56, 57},
    {81, 91, 92, 101, 102, 111, 43, 81, 46, 113, 30, 39, 37, 38, 48, 39, 78, 79, 80, 48, 67, 79, 68, 69, 70, 71, 11, 50, 67, 49, 61, 67, 67, 68, 13, 87, 106, 116, 88, 89, 98, 108, 109, 119},
    {81, 91, 92, 101, 102, 111, 72, 77, 112, 117, 41, 42, 47, 48, 30, 31, 37, 38, 5, 39, 48, 39, 78, 79, 80, 48, 67, 49, 60, 68, 66, 67, 68, 69, 13, 87, 106, 116, 59, 58, 52, 53, 57, 58, 88, 89, 98, 108, 109, 119},
    {80, 81, 90, 91, 100, 101, 77, 76, 77, 78, 40, 79, 40, 78, 30, 39, 37, 38, 60, 69, 61, 70, 62, 71, 67, 68, 13, 87, 106, 116, 88, 89, 98, 108, 109, 119},
    {79, 78, 111, 110, 77, 78, 79, 80, 72, 77, 112, 117, 31, 30, 37, 38, 59, 58, 52, 53, 57, 58, 88, 89, 98, 108, 109, 119},
    {78, 77, 112, 117, 76, 77, 60, 69, 61, 70, 62, 71},
    {80, 39, 89, 119}
};

#define NUM_FRAMES (sizeof(frames) / sizeof(frames[0]))

// ====== END CONFIGURATION ======

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

uint32_t wheel(byte pos) {
  pos = 255 - pos;
  if(pos < 85) {
    return strip.Color(255 - pos * 3, 0, pos * 3);
  }
  if(pos < 170) {
    pos -= 85;
    return strip.Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return strip.Color(pos * 3, 255 - pos * 3, 0);
}

void showFrame(int frameIndex, uint8_t baseHue) {
  // Clear
  for(int i=0; i<NUM_LEDS; i++) strip.setPixelColor(i, 0);

  // Draw frame with rainbow effect
  int n = frames[frameIndex].size();
  for (int j=0; j<n; j++) {
    int ledIdx = frames[frameIndex][j];
    uint8_t hue = baseHue + (255 * j) / n;
    strip.setPixelColor(ledIdx, wheel(hue));
  }
  strip.show();
}

void loop() {
  static uint8_t hue = 0;
  static int frame = 0;
  static unsigned long lastUpdate = 0;
  const unsigned long frameDelay = 80;  // ms per frame
  const unsigned long colorDelay = 2;   // ms per hue shift

  unsigned long now = millis();
  if (now - lastUpdate > frameDelay) {
    showFrame(frame, hue);
    frame = (frame + 1) % frames.size();
    hue += 8; // Rotate the color
    lastUpdate = now;
  }
}
