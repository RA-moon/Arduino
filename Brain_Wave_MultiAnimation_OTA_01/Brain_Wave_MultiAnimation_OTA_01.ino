#include <WiFi.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
#include "animtated_circles.h"
#include "animtated_lines.h"
#include "animtated_circles_reversed.h"
#include "animtated_lines_reversed.h"
#include "frame_interpolation.h"
#include "waveform.h"
#include "wave_position.h"
#include "audio_processor.h"
#include "animation_manager.h"

// ====== WiFi Credentials ======
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// ====== LED Configuration ======
#define DATA_PIN         17
#define NUM_LEDS         120
#define LED_TYPE         WS2812B
#define COLOR_ORDER      GRB
#define BRIGHTNESS       80
#define DELAY_MS         10
#define WAVE_INTERVAL    500
#define AUDIO_INTERVAL   20
#define MAX_ACTIVE_WAVES  10

CRGB leds[NUM_LEDS];

const float decayRate = 0.9;
unsigned long lastWaveTime = 0;
unsigned long lastAudioTime = 0;

// ====== OTA Setup Function ======
void setupOTA() {
  ArduinoOTA.setHostname("esp32-c6");
  // Optional: ArduinoOTA.setPassword("your_password");

  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]\n", error);
  });

  ArduinoOTA.begin();
}

void runLedAnimation() {
  updateAnimationSwitch();
  const auto& frames = getCurrentAnimationFrames();
  FastLED.clear();

  updateWaves();
  const auto& waves = getWaves();
  for (const auto& wave : waves) {
    auto frame = getInterpolatedFrame(
      frames,
      wave.center,
      wave.hue,
      wave.tailWidth,
      wave.noseWidth,
      BRIGHTNESS * brightnessPulse,
      wave.reverse
    );

    for (const auto& f : frame) {
      leds[f.ledIndex] = f.color;
    }
  }

  brightnessPulse *= decayRate;
  float minPulse = float(60) / BRIGHTNESS;
  if (brightnessPulse < minPulse) brightnessPulse = minPulse;

  FastLED.show();
  delay(DELAY_MS);

  unsigned long now = millis();
  if (now - lastWaveTime >= WAVE_INTERVAL) {
    if (getWaves().size() < MAX_ACTIVE_WAVES) {
      addWave(random(0, 65536), 0, 1, 2);
      lastWaveTime = now;
    }
  }
}

void setup() {
  Serial.begin(115200);

  // WiFi Connect
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  // OTA Setup
  setupOTA();

  // LED Setup
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  resetWaves();
  randomSeed(analogRead(0));

  // Audio Setup
  setupI2S();  // from audio_processor.cpp
}

void loop() {
  ArduinoOTA.handle();  // Required for OTA

  unsigned long now = millis();

  if (now - lastAudioTime >= AUDIO_INTERVAL) {
    processAudio();  // updates brightnessPulse only
    lastAudioTime = now;
  }

  runLedAnimation();
}
