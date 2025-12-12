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

// ==== WiFi Credentials ====
const char* ssid = "MIX 4";
const char* password = "Ramun123";

// ==== LED Configuration ====
#define DATA_PIN1         17
#define NUM_LEDS1         120

#define DATA_PIN2         16
#define NUM_LEDS2         44

#define TOTAL_LEDS        (NUM_LEDS1 + NUM_LEDS2)
#define SECOND_STRIP_START NUM_LEDS1

#define LED_TYPE          WS2812B
#define COLOR_ORDER       GRB
#define BRIGHTNESS        80
#define DELAY_MS          10
#define WAVE_INTERVAL     500
#define AUDIO_INTERVAL    20
#define MAX_ACTIVE_WAVES  10

CRGB leds[TOTAL_LEDS];
const float decayRate = 0.9;
unsigned long lastWaveTime = 0;
unsigned long lastAudioTime = 0;

// ==== Blinking for second strip ====
unsigned long lastBlinkTime = 0;
bool greenBlinkState = false;
const unsigned long BLINK_INTERVAL = 500;  // ms

// ==== OTA Setup ====
void setupOTA() {
  ArduinoOTA.setHostname("esp32-c6");

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

// ==== Strip 1: Audio-reactive animation ====
void runLedAnimation() {
  updateAnimationSwitch();
  const auto& frames = getCurrentAnimationFrames();
  FastLED.clear(true);  // clear both strips

  updateWaves();
  const auto& waves = getWaves();
  for (const auto& wave : waves) {
    auto frame = getInterpolatedFrame(
      frames,
      wave.center,
      wave.hue,
      wave.tailWidth,
      wave.noseWidth,
      BRIGHTNESS,  // <-- Use full brightness
      wave.reverse
    );

    for (const auto& f : frame) {
      if (f.ledIndex < NUM_LEDS1) {  // Limit to strip 1
        leds[f.ledIndex] = f.color;
      }
    }
  }

  // Apply brightnessPulse ONLY to strip 1
  for (int i = 0; i < NUM_LEDS1; i++) {
    leds[i].nscale8_video(uint8_t(brightnessPulse * 255));
  }

  brightnessPulse *= decayRate;
  float minPulse = float(60) / BRIGHTNESS;
  if (brightnessPulse < minPulse) brightnessPulse = minPulse;

  unsigned long now = millis();
  if (now - lastWaveTime >= WAVE_INTERVAL) {
    if (getWaves().size() < MAX_ACTIVE_WAVES) {
      addWave(random(0, 65536), 0, 1, 2);
      lastWaveTime = now;
    }
  }
}


// ==== Strip 2: Rainbow and Green Blink ====
void runSecondStripAnimation() {
  uint16_t rainbowHue = millis() / 10;

  // Rainbow wave on LEDs 0–32 and 40–43
  for (int i = 0; i <= 32; i++) {
    leds[SECOND_STRIP_START + i] = CHSV((rainbowHue + i * 8) % 256, 255, BRIGHTNESS);
  }
  for (int i = 40; i <= 43; i++) {
    leds[SECOND_STRIP_START + i] = CHSV((rainbowHue + i * 8) % 256, 255, BRIGHTNESS);
  }

  // Blinking green on LEDs 33–39
  unsigned long now = millis();
  if (now - lastBlinkTime >= BLINK_INTERVAL) {
    greenBlinkState = !greenBlinkState;
    lastBlinkTime = now;
  }

  CRGB blinkColor = greenBlinkState ? CRGB::Green : CRGB::Black;
  for (int i = 33; i <= 39; i++) {
    leds[SECOND_STRIP_START + i] = blinkColor;
  }
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);

  // WiFi + OTA
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  setupOTA();

  // LED Setup
  FastLED.addLeds<LED_TYPE, DATA_PIN1, COLOR_ORDER>(leds, NUM_LEDS1);
  FastLED.addLeds<LED_TYPE, DATA_PIN2, COLOR_ORDER>(leds + NUM_LEDS1, NUM_LEDS2);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // Other
  resetWaves();
  randomSeed(analogRead(0));
  setupI2S();
}

// ==== Loop ====
void loop() {
  ArduinoOTA.handle();

  unsigned long now = millis();

  if (now - lastAudioTime >= AUDIO_INTERVAL) {
    processAudio();  // updates brightnessPulse
    lastAudioTime = now;
  }

  runLedAnimation();          // Strip 1
  runSecondStripAnimation();  // Strip 2

  FastLED.show();
  delay(DELAY_MS);
}
