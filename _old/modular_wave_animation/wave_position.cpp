
#include "wave_position.h"

static float waveCenter = -1.0;
static float step = 0.2;
static float maxPos = 12.0;
static int cycle = 0;

float getNextWavePosition() {
  waveCenter += step;
  if (waveCenter > maxPos) {
    waveCenter = -1.0;
    cycle++;
  }
  return waveCenter;
}

void resetWave() {
  waveCenter = -1.0;
}

uint32_t getWaveHue() {
  return (cycle * 65536L / 50) % 65536;
}
