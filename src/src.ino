// SYSTEM LIBRARIES
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Soundboard.h>
#include <Arduino.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <WString.h>

// SYSTEM DEFINITIONS & PINS
#define SBAUD 9600
#define PIN_SFX_RST A0

// KEYPAD OBJECT
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                         {'4', '5', '6', 'B'},
                         {'7', '8', '9', 'C'},
                         {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

unsigned long loopCount;
unsigned long startTime;

// LED STRIPS
#define LA_LEN 31
#define LA_WID 3
#define LA_TOT (LA_LEN * LA_WID)
#define PIN_LED_L 10
#define PIN_LED_R 11

Adafruit_NeoPixel stripLeft =
    Adafruit_NeoPixel(LA_LEN * LA_WID, PIN_LED_L, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripRight =
    Adafruit_NeoPixel(LA_LEN * LA_WID, PIN_LED_R, NEO_GRB + NEO_KHZ800);

// Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, SFX_RST);

static const uint32_t white = Adafruit_NeoPixel::Color(255, 255, 255);
static const uint32_t v_yellow = 0xB3FF00;
static const uint32_t v_green = 0x02EF00;

// CUSTOM FUNCTIONS
void fillColumn(Adafruit_NeoPixel *s, int column, uint32_t color) {
  s->setPixelColor(column, color);
  s->setPixelColor(2 * LA_LEN - column - 1, color);
  s->setPixelColor(column + 2 * LA_LEN, color);
}
uint32_t clipByte(uint32_t x) {
  x = (x >= 255) ? 255 : x;
  return x;
}
uint32_t addColors(uint32_t c1, uint32_t c2) {
  return clipByte(((c1 & 0x000000FF) + (c2 & 0x000000FF)) >> 0) << 0 |
         clipByte(((c1 & 0x0000FF00) + (c2 & 0x0000FF00)) >> 8) << 8 |
         clipByte(((c1 & 0x00FF0000) + (c2 & 0x00FF0000)) >> 16) << 16 |
         clipByte(((c1 & 0xFF000000) + (c2 & 0xFF000000)) >> 24) << 24;
}
uint32_t multiplyColor(double m, uint32_t c1) {
  uint32_t buf = 0;
  buf |= clipByte(m * ((c1 & 0x000000FF) >> 0)) << 0;
  buf |= clipByte(m * ((c1 & 0x0000FF00) >> 8)) << 8;
  buf |= clipByte(m * ((c1 & 0x00FF0000) >> 16)) << 16;
  buf |= clipByte(m * ((c1 & 0xFF000000) >> 24)) << 24;
  return buf;
}
uint32_t addWithOpacity(uint32_t c1, double a1, uint32_t c2, double a2) {
  return addColors(multiplyColor(a1, c1), multiplyColor(a2, c2));
}
#define T 15
#define Tb 8
static uint32_t frames = 0;

// SETUP
void setup() {
  // system initializations
  Serial.begin(SBAUD);
  randomSeed(analogRead(A0) + micros()); // try to get a truly random seed

  // object initializations
  stripLeft.begin();
  stripRight.begin();
  stripLeft.clear();
  stripRight.clear();

  // brightness values are scaled later
  stripLeft.setBrightness(255);
  stripRight.setBrightness(255);
}

// LOOP
void loop() {
  double b = 0.05 + 0.08 * (0.5 + 0.5 * sin(2.0 * PI / Tb * millis() / 1000.0));
  (millis() % 10000) / 10000.0 - 0.5;
  for (int n = 0; n < LA_LEN; ++n) {
    // aprx pulse thickness, eg, 11 pixels:
    double phi = -1 * (PI)*n / 11;
    double A = 0.5 + 0.5 * sin(2.0 * PI / T * millis() / 1000.0 + phi);

    fillColumn(
        &stripLeft, n,
        multiplyColor(
            b, addWithOpacity(v_yellow, 0.23, multiplyColor(A, v_green), 1.0)));
    fillColumn(
        &stripRight, n,
        multiplyColor(b, addWithOpacity(v_yellow, 0.23,
                                        multiplyColor(1.0 - A, v_green), 1.0)));
  }
  stripLeft.show();
  stripRight.show();

  frames++;
  Serial.print("FPS: ");
  Serial.print(1000 * frames / millis());
  Serial.println();
}
