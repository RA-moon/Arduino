#include <Adafruit_NeoPixel.h>

// -------------------- Settings --------------------
#define LED_PIN      6       // Data pin to LED strip
#define LED_COUNT    300     // Number of LEDs

#define BUTTON_PIN   2       // Mode button (to GND, uses internal pullup)
#define POT1_PIN     A0      // Poti 1: width (mode 1) / brightness (mode 2)
#define POT2_PIN     A1      // Poti 2: speed (mode 1) / warmth (mode 2)

// Limit max brightness (0–255)
const uint8_t  MAX_BRIGHTNESS   = 255;

// Maximaler Schritt pro Frame für die Rainbow-Geschwindigkeit
const uint8_t  MAX_SPEED_STEP   = 10;   // zum "Tunen" einfach erhöhen oder verringern

// Debounce for button
const unsigned long DEBOUNCE_MS = 50;

// -------------------- Globals --------------------
// NEO_GRB passt bei dir (Test war Rot / Grün / Blau)
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Start: Mode 2 = White
uint8_t currentMode = 2;           // 1 = Rainbow, 2 = White
bool buttonState       = HIGH;     // current debounced state
bool lastButtonReading = HIGH;     // last raw read
unsigned long lastDebounceTime = 0;

uint16_t rainbowOffset = 0;        // animation step for rainbow

// globale Helligkeit, die in Mode 2 per Poti gesetzt
// und in Mode 1 weiterverwendet wird
uint8_t currentBrightness = MAX_BRIGHTNESS;

// Farbmischung: deine Werte
// Kalt
const uint8_t COOL_R = 255;
const uint8_t COOL_G = 255;
const uint8_t COOL_B = 200;

// Warm
const uint8_t WARM_R = 150;
const uint8_t WARM_G = 255;
const uint8_t WARM_B = 50;

// -------------------- Setup --------------------
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(POT1_PIN, INPUT);
  pinMode(POT2_PIN, INPUT);

  strip.begin();
  strip.show(); // all off
  strip.setBrightness(currentBrightness);
}

// -------------------- Main loop --------------------
void loop() {
  handleButton();  // change mode via button

  // Read potis
  int pot1 = analogRead(POT1_PIN); // 0..1023
  int pot2 = analogRead(POT2_PIN); // 0..1023

  if (currentMode == 1) {
    // MODE 1: Rainbow
    // Poti 1: Rainbow-Breite (smooth)
    uint16_t widthFactor = map(pot1, 0, 1023, 32, 2048);
    // Poti 2: Geschwindigkeit als "Offset-Schritt"
    // 0 = kein Schritt (steht), MAX_SPEED_STEP = maximale Geschwindigkeit
    uint8_t speedStep = map(pot2, 0, 1023, 0, MAX_SPEED_STEP);

    showRainbowFrame(widthFactor, speedStep);

  } else {
    // MODE 2: White light
    // Poti 1: brightness (setzt globale currentBrightness)
    uint8_t brightness = map(pot1, 0, 1023, 0, MAX_BRIGHTNESS);
    currentBrightness = brightness;     // merken für beide Modi

    // Poti 2: warmth (0 = cool, 255 = warm)
    uint8_t warmth = map(pot2, 0, 1023, 0, 255);

    showWhiteFrame(currentBrightness, warmth);

    delay(10); // base speed / CPU relief
  }
}

// -------------------- Button handling --------------------
void handleButton() {
  bool reading = digitalRead(BUTTON_PIN);  // HIGH = not pressed, LOW = pressed

  if (reading != lastButtonReading) {
    // state changed → reset debounce timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
    // stable state
    if (reading != buttonState) {
      buttonState = reading;

      // button pressed (goes LOW)
      if (buttonState == LOW) {
        // toggle mode: 1 -> 2 -> 1 -> ...
        currentMode++;
        if (currentMode > 2) {
          currentMode = 1;
        }
      }
    }
  }

  lastButtonReading = reading;
}

// -------------------- Mode 1: Rainbow frame --------------------
// widthFactor = "Dichte" des Rainbow (fein einstellbar)
// speedStep   = wie stark rainbowOffset pro Frame erhöht wird
//              0 = keine Bewegung, >0 = Bewegung, MAX_SPEED_STEP = am schnellsten
void showRainbowFrame(uint16_t widthFactor, uint8_t speedStep) {
  // Helligkeit = zuletzt in Mode 2 eingestellte Helligkeit
  strip.setBrightness(currentBrightness);

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    // Farbindex wächst mit i * widthFactor / LED_COUNT
    uint16_t colorIndex =
      (((uint32_t)i * widthFactor / strip.numPixels()) + rainbowOffset) & 255;

    strip.setPixelColor(i, wheel(colorIndex));
  }

  strip.show();

  // Geschwindigkeit: bei speedStep = 0 kein Offset -> Bild steht
  rainbowOffset += speedStep;

  // kleiner konstanter Delay für stabilen Ablauf
  delay(10);
}

// -------------------- Mode 2: White frame --------------------
void showWhiteFrame(uint8_t brightness, uint8_t warmth) {
  strip.setBrightness(brightness);

  // warmth: 0 = fully cool, 255 = fully warm
  uint8_t r = lerp8(COOL_R, WARM_R, warmth);
  uint8_t g = lerp8(COOL_G, WARM_G, warmth);
  uint8_t b = lerp8(COOL_B, WARM_B, warmth);

  uint32_t color = strip.Color(r, g, b);

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

// -------------------- Helper: 8-bit linear interpolation --------------------
uint8_t lerp8(uint8_t a, uint8_t b, uint8_t t) {
  return (uint8_t)(((uint16_t)a * (255 - t) + (uint16_t)b * t) / 255);
}

// -------------------- Color wheel for rainbow --------------------
uint32_t wheel(byte pos) {
  if (pos < 85) {
    return strip.Color(pos * 3, 255 - pos * 3, 0);
  } else if (pos < 170) {
    pos -= 85;
    return strip.Color(255 - pos * 3, 0, pos * 3);
  } else {
    pos -= 170;
    return strip.Color(0, pos * 3, 255 - pos * 3);
  }
}
