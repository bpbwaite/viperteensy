// SYSTEM LIBRARIES
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Soundboard.h>
#include <Arduino.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <WString.h>

typedef uint32_t acolor;
// SYSTEM DEFINITIONS
#define DEBUGGING true
#undef DEBUGGING
#define DEBUG_PUSH_RATE 2000 // ms
#define SBAUD 115200
#define SOFTWAREBAUD 9600

// KEYPAD OBJECT
#define PIN_SFX_RST A0
#define PINS_RESV_KPD (2 - 9) // don't use this range
#define KPD_TIMEOUT 15000     // user input timeout, ms
#define ILEN 3                // input length, in characters
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                         {'4', '5', '6', 'B'},
                         {'7', '8', '9', 'C'},
                         {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
char inputStr[(ILEN + 1)];
unsigned cPtr = 0; // character iterator
// LED STRIPS
#define SMALL_L (19 * 3)       // number of lights in the small left strip
#define SMALL_R ((17 * 3) + 3) // number of lights in the small(er) right strip
#define LA_LEN 31
#define LA_WID 3
#define LA_TOT (LA_LEN * LA_WID)
#define PIN_LED_L 10
#define PIN_LED_R 11
#define Tc 15.0 // Color cycle period, seconds
#define Tb 8.0  // Brightness cycle period, seconds

static const acolor white = Adafruit_NeoPixel::Color(255, 255, 255);
static const acolor v_yellow = 0xB3FF00;
static const acolor v_green = 0x02EF00;

Adafruit_NeoPixel stripLeft = Adafruit_NeoPixel(
    LA_LEN * LA_WID + SMALL_L, PIN_LED_L, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripRight = Adafruit_NeoPixel(
    LA_LEN * LA_WID + SMALL_R, PIN_LED_R, NEO_GRB + NEO_KHZ800);

// SOUNDBOARD
// only pins 2 and 3 support hardware edge interrupts
#define SFX_TX 11
#define SFX_RX 10 // pin must support CHANGE interrupts
#define UG LOW    // reminder: connect UG pin to ground physically!
SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard(&ss, NULL, PIN_SFX_RST);

// CUSTOM FUNCTIONS
void fillColumn(Adafruit_NeoPixel *s, int column, acolor color,
                int offset = 0) {
  // assumes columns have 3 pixels
  s->setPixelColor(offset + column, color);
  s->setPixelColor(offset + 2 * LA_LEN - column - 1, color);
  s->setPixelColor(offset + column + 2 * LA_LEN, color);
}
uint32_t clipByte(uint32_t x) {
  x = (x >= 255) ? 255 : x;
  return x;
}
acolor addColors(acolor c1, acolor c2) {
  return clipByte(((c1 & 0x000000FF) + (c2 & 0x000000FF)) >> 0) << 0 |
         clipByte(((c1 & 0x0000FF00) + (c2 & 0x0000FF00)) >> 8) << 8 |
         clipByte(((c1 & 0x00FF0000) + (c2 & 0x00FF0000)) >> 16) << 16 |
         clipByte(((c1 & 0xFF000000) + (c2 & 0xFF000000)) >> 24) << 24;
}
acolor multiplyColor(double m, acolor c1) {
  uint32_t buf = 0;
  buf |= clipByte(m * ((c1 & 0x000000FF) >> 0)) << 0;
  buf |= clipByte(m * ((c1 & 0x0000FF00) >> 8)) << 8;
  buf |= clipByte(m * ((c1 & 0x00FF0000) >> 16)) << 16;
  buf |= clipByte(m * ((c1 & 0xFF000000) >> 24)) << 24;
  return buf;
}
acolor addWithOpacity(acolor c1, double a1, acolor c2, double a2) {
  return addColors(multiplyColor(a1, c1), multiplyColor(a2, c2));
}

// CONTROLS, DEBUGGING, UTILITIES, MEMORY, & WATCHDOG
static unsigned long loops = 0;
static unsigned long toli = 0; // time of last interaction
inline void set_toli() { toli = millis(); }
inline unsigned long tsli() { return millis() - toli; }
static unsigned long tolp = 0; // time of last push
inline void set_tolp() { tolp = millis(); }
inline unsigned long tslp() { return millis() - tolp; }

// SETUP
void setup() {
// system initializations
#ifdef DEBUGGING
  Serial.begin(SBAUD);
  while (!Serial)
    ; // wait for connection when debugging
#endif

  ss.begin(SOFTWAREBAUD); // must be 9600 baud

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  if (!sfx.reset()) {
#ifdef DEBUGGING
    Serial.println("NOT FOUND");
    while (true)
      ;
#endif
  }
  sfx.volUp();

  // object initializations
  stripLeft.begin();
  stripRight.begin();
  stripLeft.clear();
  stripRight.clear();
  // disable brightness prescalar
  stripLeft.setBrightness(0xFF);
  stripRight.setBrightness(0xFF);

  set_toli();
  yield();
}

// LOOP
static unsigned long renders = 0;
static int n = 0;      // nth column of pixels in large strip
static double b = 0.0; // brightness

static char k = NO_KEY;

void loop() {
  // aprx pulse thickness, eg, 11 pixels:
  double phi = -1 * (PI)*n / 11; // negative so it flows down instead up
  double A = 0.5 + 0.5 * sin(2.0 * PI / Tc * millis() / 1000.0 + phi);
  double B = 1.0 - A;
  // apply base color:
  if (!n) {
    stripLeft.fill(multiplyColor(
        b, addWithOpacity(v_yellow, 0.23, multiplyColor(A, v_green), 1.0)));
    stripRight.fill(multiplyColor(
        b, addWithOpacity(v_yellow, 0.23, multiplyColor(B, v_green), 1.0)));
  }

  // flowing effect on larger segments:
  fillColumn(&stripLeft, n,
             multiplyColor(b, addWithOpacity(v_yellow, 0.23,
                                             multiplyColor(A, v_green), 1.0)),
             SMALL_L);
  fillColumn(&stripRight, n,
             multiplyColor(b, addWithOpacity(v_yellow, 0.23,
                                             multiplyColor(B, v_green), 1.0)),
             SMALL_R);

  if (++n >= LA_LEN) {
    stripLeft.show();
    stripRight.show();
    renders++;
    n = 0;
    b = 0.05 + 0.08 * (0.5 + 0.5 * sin(2.0 * PI / Tb * millis() / 1000.0));
  }

  if ((k = kpd.getKey()) != NO_KEY) {
    set_toli();

    if (isdigit(k)) {
      inputStr[cPtr % ILEN] = k;
    } else if (k == '*') {
      // reset input buffer and stop sound
      cPtr = 0;
      strcpy(inputStr, "---");
      sfx.stop();
    } else if (k == '#') {
      // check if it is in range? nah
      // convert string and play sound
      char tmp[8];
      strcpy(tmp, inputStr);
      strcat(tmp, ".mp3");
      sfx.playTrack(tmp);
    }
  }

  // Watchdog
  if (tsli() >= KPD_TIMEOUT) {
    strcpy(inputStr, "---");
    cPtr = 0;
  }

  loops++; // increment loop counter

#ifdef DEBUGGING
  // serial debugger for LED strips
  if (!tslp() || tslp() >= DEBUG_PUSH_RATE) {
    Serial.print("Scans: ");
    Serial.print(1000UL * loops / millis());
    Serial.print("/s, Display: "); // partial FPS average
    Serial.print(1000UL * renders / millis());
    Serial.print(" FPS");
    Serial.println();
    set_tolp();
  }
#endif
}
