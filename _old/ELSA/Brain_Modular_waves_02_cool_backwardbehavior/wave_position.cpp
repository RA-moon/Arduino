#include "wave_position.h"
#include <vector>

static std::vector<Wave> waves;

void resetWaves() {
  waves.clear();
}

void updateWaves() {
  for (auto& wave : waves) {
    wave.center += wave.speed;
    if (wave.center > 12.0f + wave.noseWidth) wave.center = -wave.tailWidth;
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
  w.center = -tail;
  w.speed = speed;
  w.hue = hue;
  w.noseWidth = nose;
  w.tailWidth = tail;
  w.totalWidth = nose + tail;
  w.reverse = reverse;
  waves.push_back(w);
}


