#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define PIN 5
#define MATRIX_WIDTH 18
#define MATRIX_HEIGHT 18

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH, MATRIX_HEIGHT, PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS,  // No zigzag
  NEO_GRB + NEO_KHZ800
);

void setup() {
  matrix.begin();
  matrix.setBrightness(40);  // Safe brightness level
  matrix.fillScreen(0);
  matrix.show();
}

void loop() {
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      matrix.fillScreen(0);
      matrix.drawPixel(x, y, matrix.Color(0, 255, 0));  // Green pixel
      matrix.show();
      delay(100);  // Adjust speed if needed
    }
  }
}
