// ESP32-C6, 1 NeoPixel-Strip
#include <Adafruit_NeoPixel.h>

#define LED_PIN     2
#define LED_COUNT   180
#define BRIGHTNESS  100

const int BTN_PINS[] = {0,1,6,7,4,5,8,9};   // nimm 4 oder 8
const int BTN_COUNT  = 8;                   // auf 4 setzen, wenn nur 4 Tasten

// Zuordnung Taste -> LED (anpassen). Werte < LED_COUNT.
int MAP_BTN_TO_LED[BTN_COUNT] = {0,1,2,3,4,5,6,7};

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  for (int i=0;i<BTN_COUNT;i++) pinMode(BTN_PINS[i], INPUT_PULLUP); // Taster nach GND
}

uint32_t onColor(uint8_t i){
  // pro Taste andere Farbe; alternativ: return strip.Color(255,255,255);
  return strip.gamma32(strip.ColorHSV((uint32_t)i*65535/BTN_COUNT, 255, 255));
}

void loop() {
  strip.clear();
  for (int i=0;i<BTN_COUNT;i++){
    if (digitalRead(BTN_PINS[i]) == LOW) {
      int led = MAP_BTN_TO_LED[i];
      if (led >= 0 && led < LED_COUNT) strip.setPixelColor(led, onColor(i));
    }
  }
  strip.show();
  delay(5); // einfache Entprellung
}
