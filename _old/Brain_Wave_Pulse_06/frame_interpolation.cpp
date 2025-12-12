#include "frame_interpolation.h"
#include "waveform.h"
#include <FastLED.h>

std::vector<FrameResult> getInterpolatedFrame(
  const std::vector<std::vector<int>>& frames,
  float waveCenter,
  uint32_t baseHue,
  float widthBehind,
  float widthAhead,
  int brightness,
  bool reverse
) {
  std::vector<FrameResult> result;
  int totalRings = frames.size();

  for (int i = 0; i < totalRings; i++) {
    int waveFrameIndex = i;
    int brightnessFrameIndex = reverse ? (totalRings - 1 - i) : i;

    float actualTail = reverse ? widthAhead : widthBehind;
    float actualNose = reverse ? widthBehind : widthAhead;

    float intensity = getAsymmetricIntensity(
      brightnessFrameIndex, waveCenter, actualTail, actualNose
    );

    if (intensity < 0.001f) continue;

    CRGB color = CHSV(baseHue >> 8, 255, 255);
    color.nscale8_video(brightness * intensity);  // full color then fade

    for (int led : frames[waveFrameIndex]) {
      if (led >= 0 && led < 120) {
        FrameResult fr;
        fr.ledIndex = led;
        fr.color = color;
        result.push_back(fr);
      }
    }
  }

  return result;
}
