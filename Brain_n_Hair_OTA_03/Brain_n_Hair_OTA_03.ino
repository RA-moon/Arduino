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
//#include "audio_processor.h"
#include "animation_manager.h"

// ==== WiFi Credentials ====
const char* ssid = "MIX 4";
const char* password = "Ramun123";

// ==== LED Configuration ====
#define DATA_PIN1         17
#define NUM_LEDS1         120

#define DATA_PIN2         16
#define NUM_LEDS2         44

#define LED_TYPE          WS2812B
#define COLOR_ORDER       GRB
#define BRIGHTNESS1       80
#define DELAY_MS          10
#define WAVE_INTERVAL     500
#define AUDIO_INTERVAL    20
#define MAX_ACTIVE_WAVES  10

CRGB leds1[NUM_LEDS1];
CRGB leds2[NUM_LEDS2];

float brightnessPulse = 1.0;  // Actual definition here

const float decayRate = 0.9;
unsigned long lastWaveTime = 0;
unsigned long lastAudioTime = 0;
unsigned long lastBlinkTime = 0;
bool greenBlinkState = false;
const unsigned long BLINK_INTERVAL = 500;

// ==== OTA Setup ====
void setupOTA() {
  ArduinoOTA.setHostname("esp32-c6");
  ArduinoOTA.onStart([]() { Serial.println("OTA Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("OTA End"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]\n", error);
  });
  ArduinoOTA.begin();
}

// ==== Strip 1 ====
void runLedAnimation() {
  updateAnimationSwitch();
  const auto& frames = getCurrentAnimationFrames();
  fill_solid(leds1, NUM_LEDS1, CRGB::Black);

  updateWaves();
  const auto& waves = getWaves();
  for (const auto& wave : waves) {
    auto frame = getInterpolatedFrame(
      frames,
      wave.center,
      wave.hue,
      wave.tailWidth,
      wave.noseWidth,
      BRIGHTNESS1,
      wave.reverse
    );

    for (const auto& f : frame) {
      if (f.ledIndex < NUM_LEDS1) {
        leds1[f.ledIndex] = f.color;
      }
    }
  }

  for (int i = 0; i < NUM_LEDS1; i++) {
    leds1[i].nscale8_video(uint8_t(brightnessPulse * 255));
  }

  brightnessPulse *= decayRate;
  float minPulse = float(60) / BRIGHTNESS1;
  if (brightnessPulse < minPulse) brightnessPulse = minPulse;

  unsigned long now = millis();
  if (now - lastWaveTime >= WAVE_INTERVAL && getWaves().size() < MAX_ACTIVE_WAVES) {
    addWave(random(0, 65536), 0, 1, 2);
    lastWaveTime = now;
  }
}

// ==== Strip 2 ====
void runSecondStripAnimation() {
  uint16_t rainbowHue = millis() / 10;

  for (int i = 0; i <= 32; i++) {
    leds2[i] = CHSV((rainbowHue + i * 8) % 256, 255, 255);
  }
  for (int i = 40; i <= 43; i++) {
    leds2[i] = CHSV((rainbowHue + i * 8) % 256, 255, 255);
  }

  unsigned long now = millis();
  if (now - lastBlinkTime >= BLINK_INTERVAL) {
    greenBlinkState = !greenBlinkState;
    lastBlinkTime = now;
  }

  CRGB blinkColor = greenBlinkState ? CRGB::Green : CRGB::Black;
  for (int i = 33; i <= 39; i++) {
    leds2[i] = blinkColor;
  }
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  setupOTA();

  FastLED.addLeds<LED_TYPE, DATA_PIN1, COLOR_ORDER>(leds1, NUM_LEDS1).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, DATA_PIN2, COLOR_ORDER>(leds2, NUM_LEDS2).setCorrection(TypicalLEDStrip);

  FastLED.setBrightness(BRIGHTNESS1);  // Controls only leds1
  FastLED.clear(true);

  resetWaves();
  randomSeed(esp_random());
  // setupI2S();
}

// ==== Loop ====
void loop() {
  ArduinoOTA.handle();

  unsigned long now = millis();
  // if (now - lastAudioTime >= AUDIO_INTERVAL) {
  //   processAudio();
  //   lastAudioTime = now;
  // }

  runLedAnimation();
  runSecondStripAnimation();

  FastLED[0].showLeds(BRIGHTNESS1);  // Strip 1 with dimming
  FastLED[1].showLeds(255);          // Strip 2 always full brightness

  delay(DELAY_MS);
}
