#include "animtated_circles.h"

void setup() {
  // Initialize LEDs, e.g., FastLED.addLeds<...>();
}

void loop() {
  vector<vector<int>> frames = getAnimationFrames();

  for (auto& frame : frames) {
    clearAllLeds();
    for (int id : frame) {
      setLedOn(id);  // Your LED control logic
    }
    showLeds();
    delay(100);
  }
}

// Stub implementations
void clearAllLeds() {
  // fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void setLedOn(int id) {
  // leds[id] = CRGB::White;
}

void showLeds() {
  // FastLED.show();
}
