
#include "waveform.h"
#include <math.h>

float getAsymmetricIntensity(float frameIndex, float center, float widthBehind, float widthAhead) {
  float d = frameIndex - center;
  float norm = (d < 0) ? (d / widthBehind) : (d / widthAhead);
  return exp(-norm * norm);
}
