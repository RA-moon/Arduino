// Zwei NeoPixel-Strips: A=180 LEDs an Pin 1, B=120 LEDs an Pin 2
// Buttons: 0(-),1(+),4=grün,5=gelb,6=weiß,7=blau,8=rot,9=Regenbogen
#include <Adafruit_NeoPixel.h>

#define LED_PIN_A   12
#define LED_CNT_A   180
#define LED_PIN_B   13
#define LED_CNT_B   120

// Buttons (gegen GND), interner Pullup
#define BTN_0 0
#define BTN_1 1
#define BTN_4 4
#define BTN_5 5
#define BTN_6 6
#define BTN_7 7
#define BTN_8 8
#define BTN_9 9

#define FRAME_MS    10
#define STEP_HUE    256

// Helligkeit 20..200 in 20er Schritten
#define B_MIN   20
#define B_MAX   200
#define B_STEP  20

Adafruit_NeoPixel stripA(LED_CNT_A, LED_PIN_A, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripB(LED_CNT_B, LED_PIN_B, NEO_GRB + NEO_KHZ800);

enum Mode { MODE_RAINBOW, MODE_STATIC };
Mode mode = MODE_RAINBOW;

uint16_t baseHue = 0;
uint8_t brightness = 100;
uint32_t staticColor = 0;

unsigned long lastFrame = 0;

const uint8_t btnPins[] = { BTN_0, BTN_1, BTN_4, BTN_5, BTN_6, BTN_7, BTN_8, BTN_9 };
uint8_t prevState[8];

bool fell(uint8_t idx) {
  uint8_t s = digitalRead(btnPins[idx]);
  bool f = (prevState[idx] == HIGH && s == LOW);
  prevState[idx] = s;
  return f;
}

void setBrightnessAll(uint8_t b) {
  stripA.setBrightness(b);
  stripB.setBrightness(b);
}

void showAll() {
  stripA.show();
  stripB.show();
}

void fillAll(uint32_t c) {
  stripA.fill(c, 0, LED_CNT_A);
  stripB.fill(c, 0, LED_CNT_B);
}

void applyStatic(uint32_t c) {
  mode = MODE_STATIC;
  // gamma-korrigiert
  staticColor = stripA.gamma32(c);
  fillAll(staticColor);
  showAll();
}

void setup() {
  stripA.begin();
  stripB.begin();
  setBrightnessAll(brightness);
  showAll();

  for (uint8_t i = 0; i < 8; i++) {
    pinMode(btnPins[i], INPUT_PULLUP);
    prevState[i] = digitalRead(btnPins[i]);
  }
}

void loop() {
  // Buttons
  if (fell(0)) { // -
    int b = brightness - B_STEP; if (b < B_MIN) b = B_MIN;
    brightness = b; setBrightnessAll(brightness); showAll();
  }
  if (fell(1)) { // +
    int b = brightness + B_STEP; if (b > B_MAX) b = B_MAX;
    brightness = b; setBrightnessAll(brightness); showAll();
  }

  if (fell(2)) applyStatic(stripA.Color(0, 255, 0));       // grün
  if (fell(3)) applyStatic(stripA.Color(255, 255, 0));     // gelb
  if (fell(4)) applyStatic(stripA.Color(255, 255, 255));   // weiß
  if (fell(5)) applyStatic(stripA.Color(0, 0, 255));       // blau
  if (fell(6)) applyStatic(stripA.Color(255, 0, 0));       // rot
  if (fell(7)) mode = MODE_RAINBOW;                        // Regenbogen

  // Frames
  unsigned long now = millis();
  if (now - lastFrame >= FRAME_MS) {
    lastFrame = now;

    if (mode == MODE_RAINBOW) {
      // Strip A: 180er Regenbogen
      for (uint16_t i = 0; i < LED_CNT_A; i++) {
        uint16_t hueA = (uint32_t)i * 65535 / LED_CNT_A;
        uint32_t cA = stripA.ColorHSV(hueA + baseHue, 255, 255);
        stripA.setPixelColor(i, stripA.gamma32(cA));
      }
      // Strip B: 120er Regenbogen
      for (uint16_t i = 0; i < LED_CNT_B; i++) {
        uint16_t hueB = (uint32_t)i * 65535 / LED_CNT_B;
        uint32_t cB = stripB.ColorHSV(hueB + baseHue, 255, 255);
        stripB.setPixelColor(i, stripB.gamma32(cB));
      }
      showAll();
      baseHue += STEP_HUE;
    } else {
      fillAll(staticColor);
      showAll();
    }
  }
}
