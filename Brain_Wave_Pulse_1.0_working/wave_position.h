
#pragma once
#include <stdint.h>
#include <vector>

struct Wave {
  float center;
  float speed;
  uint32_t hue;
  float noseWidth;
  float tailWidth;
  float totalWidth;
  bool reverse;
};

void resetWaves();
void updateWaves();
const std::vector<Wave>& getWaves();
void addWave(uint32_t hue, int8_t speedControl = 0, float nose = 1.0, float tail = 3.0, bool reverse = false);
