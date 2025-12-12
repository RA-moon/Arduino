
#include "frame_interpolation.h"
#include "waveform.h"

std::vector<FrameResult> getInterpolatedFrame(
  const std::vector<std::vector<int>>& frames,
  float waveCenter,
  uint32_t baseHue,
  float widthBehind,
  float widthAhead,
  int brightness,
  bool reverse) {
  std::vector<FrameResult> result;
  int totalRings = frames.size();

  for (int i = 0; i < totalRings; i++) {
    int frameIndex = i;
    int waveFrameIndex = reverse ? (totalRings - 1 - i) : i;

    float intensity = getAsymmetricIntensity(waveFrameIndex, waveCenter, widthBehind, widthAhead);

    if (intensity < 0.01) continue;

    uint8_t value = brightness * intensity;
    uint32_t color = Adafruit_NeoPixel::ColorHSV(baseHue, 255, value);

    for (int led : frames[frameIndex]) {
      if (led >= 0 && led < 120) {
        result.push_back({ led, color });
      }
    }
  }

  return result;
}
