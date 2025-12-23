#include "audio_processor.h"
#include <Arduino.h>

// Baseline pulse multiplier.
float brightnessPulse = 1.0f;

void setupI2S() {
  // No-op placeholder.
  // If you have an I2S microphone and want real audio reactivity,
  // implement the I2S setup + sampling here.
}

void processAudio() {
  // Minimal "fake" audio envelope so the animation breathes.
  // Replace with real I2S amplitude extraction if needed.
  static unsigned long lastKickMs = 0;
  const unsigned long now = millis();

  // Occasionally create a spike.
  if (now - lastKickMs > 120UL && (uint8_t)random(0, 100) < 6) {
    brightnessPulse = 1.6f;
    lastKickMs = now;
  }

  // Ensure we never fall below 1.0 (the main loop will decay it).
  if (brightnessPulse < 1.0f) brightnessPulse = 1.0f;
}
