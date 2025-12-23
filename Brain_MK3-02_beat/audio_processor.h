#pragma once

// Simple audio pulse signal used by the LED engine.
//
// In the original project this was driven by an I2S microphone. In this fixed
// build we keep it minimal so the sketch compiles and runs on ESP32-C6 even
// without audio hardware.

extern float brightnessPulse;

void setupI2S();
void processAudio();

// Beat detection event from FFT analysis.
// Returns true once per detected beat (edge triggered).
bool consumeBeat(float* strength = nullptr);
