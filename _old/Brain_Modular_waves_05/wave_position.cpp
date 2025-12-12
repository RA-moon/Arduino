#include "wave_position.h"

static std::vector<Wave> waves;

// Hardcoded frame count (number of rings)
const float maxFrameIndex = 10.0f;

void resetWaves() {
  waves.clear();
}

void updateWaves() {
  for (auto& wave : waves) {
    wave.center += wave.speed;

    // Reset wave if it leaves visible + tail/nose space
    if ((!wave.reverse && wave.center > maxFrameIndex + wave.noseWidth) ||
        (wave.reverse && wave.center < -wave.tailWidth)) {
      wave.center = wave.reverse
        ? (maxFrameIndex + wave.noseWidth)
        : -wave.tailWidth;
    }
  }
}

const std::vector<Wave>& getWaves() {
  return waves;
}

void addWave(uint32_t hue, int8_t speedControl, float nose, float tail, bool reverse) {
  float speed = 0.2 + (speedControl / 25.0);
  if (speed < 0.1) speed = 0.1;
  if (speed > 0.6) speed = 0.6;

  Wave w;
  w.speed = reverse ? -speed : speed;
  w.hue = hue;
  w.noseWidth = nose;
  w.tailWidth = tail;
  w.totalWidth = nose + tail;
  w.reverse = reverse;

  w.center = reverse
    ? (maxFrameIndex + w.noseWidth)
    : -w.tailWidth;

  waves.push_back(w);
}
