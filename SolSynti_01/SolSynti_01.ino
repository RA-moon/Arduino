// Arduino UNO/Nano: 8-button sine generator to a small speaker on D9 (OC1A)
// Wiring:
//   D9 -> 220uF(+) -> speaker -> GND  (optional 33-100R in series with speaker)
//   Buttons: one side to pin, other side to GND; uses INPUT_PULLUP.
//
// Buttons (held = play):
//   D2,D3,D4,D5,D6,D7,D8,D10 -> C4,D4,E4,F4,G4,A4,B4,C5
//
// Uses Timer1: 8-bit Fast PWM at ~31.25 kHz on D9
// Uses Timer2: 20 kHz interrupt to step 32-bit DDS phase and update PWM duty.

#include <Arduino.h>

static const float PI_F = 3.14159265358979323846f;

const uint32_t FS = 20000;       // sine update rate (Hz)
volatile uint32_t phase = 0;     // 32-bit DDS accumulator
volatile uint32_t phaseInc = 0;  // step per sample
volatile uint8_t amplitude = 0;  // 0..255 (start silent)

uint8_t sineLUT[256];

// ----- Button setup -----
const uint8_t BTN_PINS[8] = {2, 3, 4, 5, 6, 7, 8, 10};
const float    BTN_FREQS[8] = {
  261.63f, // C4
  293.66f, // D4
  329.63f, // E4
  349.23f, // F4
  392.00f, // G4
  440.00f, // A4
  493.88f, // B4
  523.25f  // C5
};

int8_t debouncedIndex = -1;
int8_t rawIndex = -1;
unsigned long lastChangeMs = 0;
const unsigned long DEBOUNCE_MS = 10;

// ----- DDS control -----
void setFrequency(float f) {
  noInterrupts();
  phaseInc = (uint32_t)((f * 4294967296.0) / (double)FS); // f * 2^32 / FS
  interrupts();
}

void setAmplitude(uint8_t a) {
  noInterrupts();
  amplitude = a; // 0..255
  interrupts();
}

// ----- Timer2 ISR: 20 kHz update -----
ISR(TIMER2_COMPA_vect) {
  phase += phaseInc;
  uint8_t idx = phase >> 24;                       // top 8 bits
  uint16_t v  = ((uint16_t)sineLUT[idx] * amplitude) >> 8; // 0..255
  OCR1A = (uint8_t)v;                              // PWM duty to D9 (OC1A)
}

void setupTimers() {
  // Timer1 -> Fast PWM 8-bit, non-inverting on OC1A (D9), clk/1 (~31.25 kHz PWM)
  pinMode(9, OUTPUT);
  TCCR1A = _BV(COM1A1) | _BV(WGM10);
  TCCR1B = _BV(WGM12)  | _BV(CS10);
  OCR1A  = 127;

  // Timer2 -> CTC at FS (20 kHz) to step the sine
  TCCR2A = _BV(WGM21);            // CTC
  TCCR2B = _BV(CS21);             // prescaler /8
  OCR2A  = (F_CPU / (8UL * FS)) - 1; // 16e6/(8*20000)-1 = 99
  TIMSK2 = _BV(OCIE2A);           // enable compare A interrupt
}

// Return index [0..7] of the first pressed button, or -1 if none
int8_t readRawPressedIndex() {
  for (uint8_t i = 0; i < 8; i++) {
    if (digitalRead(BTN_PINS[i]) == LOW) return (int8_t)i; // active LOW
  }
  return -1;
}

void setup() {
  // Build 256-sample sine table (0..255)
  for (int i = 0; i < 256; i++) {
    float s = sinf(2.0f * PI_F * (float)i / 256.0f);
    int val = (int)lroundf(127.5f + 127.5f * s);
    sineLUT[i] = (uint8_t)val;
  }

  // Buttons with internal pull-ups
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(BTN_PINS[i], INPUT_PULLUP);
  }

  setupTimers();
  setAmplitude(0);      // start silent
  setFrequency(440.0f); // default; changed on press
}

void loop() {
  // --- debounce: choose first pressed button, else -1 ---
  int8_t r = readRawPressedIndex();
  if (r != rawIndex) {
    rawIndex = r;
    lastChangeMs = millis();
  }
  if ((millis() - lastChangeMs) >= DEBOUNCE_MS && debouncedIndex != rawIndex) {
    debouncedIndex = rawIndex;

    if (debouncedIndex >= 0) {
      setFrequency(BTN_FREQS[debouncedIndex]);
      setAmplitude(200); // play
    } else {
      setAmplitude(0);   // silence when no buttons held
    }
  }

  // (Optional) add features here: LED per key, serial monitor, etc.
}
