#include <Adafruit_NeoPixel.h>

// =======================
// Hardware-Konfiguration
// =======================
#define LED_PIN     6        // DATA-Pin für den NeoPixel-Strip -> anpassen falls nötig
#define LED_COUNT   132      // Anzahl LEDs

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// =======================
// Modi
// =======================
#define MODE_RAINBOW    0
#define MODE_WARMWHITE  1
#define MODE_BREATH     2
#define MODE_CHAOS      3
#define NUM_MODES       4

uint8_t  currentMode     = MODE_RAINBOW;
bool     modeJustChanged = true;
unsigned long lastModeSwitch = 0;
const unsigned long MODE_DURATION = 20000UL;  // 20 Sekunden

// =======================
// Rainbow (ähnlich Deinem Sketch)
// =======================
uint16_t rainbowBaseHue      = 0;        // 0..65535
unsigned long lastRainbowUpdate = 0;
const unsigned long RAINBOW_INTERVAL = 30;  // Geschwindigkeit (ms)

// =======================
// Breath (Atmen)
// =======================
uint8_t breathBrightness        = 0;
bool    breathUp                = true;
unsigned long lastBreathUpdate  = 0;
const unsigned long BREATH_INTERVAL = 30;   // Schrittzeit (ms)
const uint8_t BREATH_MIN = 10;
const uint8_t BREATH_MAX = 255;

// Warmweiß-Basisfarbe (leicht warm)
const uint8_t WARM_R = 255;
const uint8_t WARM_G = 180;
const uint8_t WARM_B = 80;

// =======================
// Chaos
// =======================
unsigned long lastChaosUpdate = 0;
const unsigned long CHAOS_INTERVAL = 50;   // ms

// =======================
// Hilfsfunktionen
// =======================

void fillWarmWhite() {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, WARM_R, WARM_G, WARM_B);
  }
}

// Rainbow-Update (laufender Regenbogen über den ganzen Strip)
void updateRainbow(unsigned long now) {
  if (modeJustChanged) {
    rainbowBaseHue = 0;
    lastRainbowUpdate = now;
    strip.setBrightness(255);
    modeJustChanged = false;
  }

  if (now - lastRainbowUpdate < RAINBOW_INTERVAL) return;
  lastRainbowUpdate = now;

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    uint16_t hue = (uint32_t)i * 65535UL / LED_COUNT;
    uint32_t c = strip.ColorHSV(hue + rainbowBaseHue, 255, 255);
    strip.setPixelColor(i, strip.gamma32(c));
  }
  strip.show();

  // Schrittweite für das "Laufen" des Regenbogens
  rainbowBaseHue += 256;
}

// Konstantes warmweißes Licht
void updateWarmWhite(unsigned long now) {
  if (modeJustChanged) {
    strip.setBrightness(255);
    fillWarmWhite();
    strip.show();
    modeJustChanged = false;
  }
  // kein weiteres Update nötig
}

// Atmen (warmweiß langsam ein- und ausblenden)
void updateBreath(unsigned long now) {
  if (modeJustChanged) {
    breathBrightness = BREATH_MIN;
    breathUp = true;
    strip.setBrightness(breathBrightness);
    fillWarmWhite();
    strip.show();
    lastBreathUpdate = now;
    modeJustChanged = false;
  }

  if (now - lastBreathUpdate < BREATH_INTERVAL) return;
  lastBreathUpdate = now;

  if (breathUp) {
    if (breathBrightness < BREATH_MAX) {
      breathBrightness++;
    } else {
      breathUp = false;
    }
  } else {
    if (breathBrightness > BREATH_MIN) {
      breathBrightness--;
    } else {
      breathUp = true;
    }
  }

  strip.setBrightness(breathBrightness);
  strip.show();
}

// Chaos (zufällige Farben, flackernd)
void updateChaos(unsigned long now) {
  if (modeJustChanged) {
    strip.setBrightness(255);
    randomSeed(analogRead(A0));  // falls A0 frei ist
    lastChaosUpdate = now;
    modeJustChanged = false;
  }

  if (now - lastChaosUpdate < CHAOS_INTERVAL) return;
  lastChaosUpdate = now;

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    uint8_t r = random(0, 256);
    uint8_t g = random(0, 256);
    uint8_t b = random(0, 256);
    strip.setPixelColor(i, r, g, b);
  }
  strip.show();
}

// =======================
// Setup & Loop
// =======================

void setup() {
  strip.begin();
  strip.setBrightness(255);
  strip.show(); // alle aus
}

void loop() {
  unsigned long now = millis();

  // Alle 20 Sekunden Modus wechseln
  if (now - lastModeSwitch >= MODE_DURATION) {
    lastModeSwitch = now;
    currentMode = (currentMode + 1) % NUM_MODES;
    modeJustChanged = true;
  }

  // Aktuellen Modus updaten
  switch (currentMode) {
    case MODE_RAINBOW:
      updateRainbow(now);
      break;

    case MODE_WARMWHITE:
      updateWarmWhite(now);
      break;

    case MODE_BREATH:
      updateBreath(now);
      break;

    case MODE_CHAOS:
      updateChaos(now);
      break;
  }
}
