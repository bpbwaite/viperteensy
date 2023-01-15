// SYSTEM LIBRARIES
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Soundboard.h>
#include <Arduino.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <WString.h>
// todo: green ripples when running ultimate sound effect
// unused pins:
// D12
// A1, A4 thru A6
typedef uint32_t acolor;
// SYSTEM DEFINITIONS
// current debug usage: 179 bytes
// #define DEBUGGING true
#define DEBUG_PUSH_RATE 1100 // T, ms
#define SBAUD 115200
#define SOFTWAREBAUD 9600

// KEYPAD OBJECT
#define PIN_SFX_RST A0
#define PINS_RESV_KPD (2 - 9) // don't use this range
#define KPD_TIMEOUT 15000     // user input timeout, ms
#define ILEN 3                // input length, in characters
static const byte ROWS = 4;   // make const bytes if memory allows
static const byte COLS = 4;   // make const bytes if memory allows
const char keys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                               {'4', '5', '6', 'B'},
                               {'7', '8', '9', 'C'},
                               {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
static char k = NO_KEY;
char inputStr[(ILEN + 1)];
char inputLast[(ILEN + 1)];
static const char fivezeros[6] = "00000";
static const char fext[4] = "OGG";
unsigned cPtr = 0; // character iterator
// special inputs and handlers
static const char toggleLED[4] = "999";
static boolean isLit = true;
static const char sexnumber[4] = "696";
static const char angry[4] = "777";

// LED STRIPS
#define SMALL_L (19 * 3)       // number of lights in the small left strip
#define SMALL_R ((17 * 3) + 3) // number of lights in the small(er) right strip
#define LA_LEN 31
#define LA_WID 3
#define LA_TOT (LA_LEN * LA_WID)
#define PIN_LED_L A2
#define PIN_LED_R A3
#define Tc 15.0 // Color cycle period, seconds
#define Tb 8.0  // Brightness cycle period, seconds

#define white 0xFFFFFF // Adafruit_NeoPixel::Color(255, 255, 255);
#define v_yellow 0xB3FF02
#define v_green 0x02EF00
#define v_red 0xFF0505
// using 1188 bytes ram for pixels
Adafruit_NeoPixel pixelmem = Adafruit_NeoPixel(LA_LEN * LA_WID + SMALL_L,
                                               PIN_LED_L, NEO_GRB + NEO_KHZ800);

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
// LOOP VARS
static unsigned scans = 0;  // kpd scans in the current debug push cycle
static unsigned n = 0;      // nth column of pixels in large strip
static double bscale = 0.0; // brightness
static double phi, A, B;
char fname[12] = "";
void resetInputStr() { strcpy(inputStr, "---"); }

void ultStrobe() {
  for (int j = 0; j < 10; ++j) {
    if (!isLit)
      return;

    pixelmem.setPin(PIN_LED_L);
    pixelmem.clear();
    pixelmem.show();
    pixelmem.setPin(PIN_LED_R);
    pixelmem.clear();
    pixelmem.show();
    delay(100);
    pixelmem.setPin(PIN_LED_L);
    pixelmem.fill(v_green);
    pixelmem.show();
    pixelmem.setPin(PIN_LED_R);
    pixelmem.fill(v_green);
    pixelmem.show();
    delay(200);
  }
}
void goAngry() {
  if (!isLit)
    return;

  pixelmem.setPin(PIN_LED_L);
  pixelmem.fill(v_red);
  pixelmem.show();
  pixelmem.setPin(PIN_LED_R);
  pixelmem.fill(v_red);
  pixelmem.show();
  delay(2900);
}
void playRandom() {
  static const short rs[] = {94,  95,  96,  273, 274, 275, 276, 277, 278,
                             279, 280, 281, 282, 283, 284, 285, 286, 287,
                             288, 305, 306, 315, 316, 349, 350, 351};
  short r = rs[random(3, 25)]; // only works with 3 digit for now
  strcpy(fname, fivezeros);
  strcat(fname, String(r).c_str());
  strcat(fname, fext);

  sfx.playTrack(fname);
}
inline void delayRandom() { delay(random(2000, 7000)); }

void kpd_scan_callback() {
  static const char spl1[] = "388";
  static const char spl2[] = "393";

  ++scans;
  if ((k = kpd.getKey()) != NO_KEY) {
    set_toli();

    if (isdigit(k)) {
      inputStr[cPtr % ILEN] = k;
      cPtr += 1;
    } else if (k == '*') {
      // reset input buffer and stop sound
      cPtr = 0;
      resetInputStr();
      sfx.stop();
    } else if (k == '#') {
      // in the future, convert the input to an int and then back when needed,
      // this is too hard check or make input valid, convert string and play

      // sound
      while (inputStr[2] == '-') {
        inputStr[2] = inputStr[1];
        inputStr[1] = inputStr[0];
        inputStr[0] = '0';
      }
      // special triggers:
      if (!strcmp(inputStr, sexnumber))
        while ((k = kpd.getKey()) != '*') {
          playRandom();
          delayRandom();
        }
      if (!strcmp(inputStr, toggleLED))
        isLit = !isLit;

      if (!atoi(inputStr)) {
        strcpy(inputStr, inputLast);
      } else
        strcpy(inputLast, inputStr);
      strcpy(fname, fivezeros);
      strcat(fname, inputStr);
      strcat(fname, fext);

      if (!sfx.playTrack(fname)) {
        delay(102); // sometimes its not ready
        sfx.playTrack(fname);
      }
      // special lighting effects
      if (!strcmp(inputStr, spl1) || !strcmp(inputStr, spl2)) {
        ultStrobe();
      }
      if (!strcmp(inputStr, angry)) {
        goAngry();
      }
      resetInputStr();
      cPtr = 0; // immediately ready for next

#ifdef DEBUGGING
      Serial.print(F("Requesting file: "));
      Serial.println(fname);
#endif
    }
  }
}

// SETUP
void setup() {
  yield(); // may reboot as programmer triggers reset
// system initializations
#ifdef DEBUGGING
  Serial.begin(SBAUD);
  while (!Serial && millis() < KPD_TIMEOUT)
    ; // wait for connection when debugging, unless there isn't one
  Serial.println(F("Entering Setup, Serial Interface Connected"));
#endif

  ss.begin(SOFTWAREBAUD); // must be 9600 baud

  if (!sfx.reset()) {
#ifdef DEBUGGING
    Serial.println(F("NO SOUNDBOARD FOUND"));
#endif
  }
#ifdef DEBUGGING
  Serial.println(F("Soundboard Connected"));
#endif

  // object initializations
  pixelmem.begin();
  pixelmem.clear();
  // disable brightness prescalar
  pixelmem.setBrightness(0xFF);
#ifdef DEBUGGING
  Serial.print(F("LED Strips Initialized: "));
  Serial.print(pixelmem.numPixels());
  Serial.println();
#endif

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(A7, INPUT);
  randomSeed(micros() + analogRead(A7)); // use noise
  resetInputStr();

  set_toli();
  yield();
}

void loop() {
  // aprx pulse thickness, eg, 11 pixels:

  boolean workingLeft = ((millis() % 34) >= 17);
  // choose which strip to render
  pixelmem.setPin((workingLeft) ? PIN_LED_L : PIN_LED_R);

  // base color on small strips and flowing effect on larger segments:
  if (workingLeft) {
    pixelmem.fill(
        multiplyColor(bscale, addWithOpacity(v_yellow, 0.23,
                                             multiplyColor(A, v_green), 1.0)),
        0x0, SMALL_L);
    for (n = 0; n < LA_LEN; ++n) {

      kpd_scan_callback();

      phi = -1 * (PI)*n / 11; // negative so it flows down instead up
      A = 0.5 + 0.5 * sin(2.0 * PI / Tc * millis() / 1000.0 + phi);
      B = 1.0 - A;
      fillColumn(
          &pixelmem, n,
          multiplyColor(bscale, addWithOpacity(v_yellow, 0.23,
                                               multiplyColor(A, v_green), 1.0)),
          SMALL_L);
    }
  } else {
    pixelmem.fill(
        multiplyColor(bscale, addWithOpacity(v_yellow, 0.23,
                                             multiplyColor(B, v_green), 1.0)),
        0x0, SMALL_R);
    for (n = 0; n < LA_LEN; ++n) {

      kpd_scan_callback();

      phi = -1 * (PI)*n / 11; // negative so it flows down instead up
      A = 0.5 + 0.5 * sin(2.0 * PI / Tc * millis() / 1000.0 + phi);
      B = 1.0 - A;
      fillColumn(
          &pixelmem, n,
          multiplyColor(bscale, addWithOpacity(v_yellow, 0.23,
                                               multiplyColor(B, v_green), 1.0)),
          SMALL_R);
    }
  }
  if (!isLit)
    pixelmem.clear();
  pixelmem.show(); // write to port

  bscale = 0.05 + 0.08 * (0.5 + 0.5 * sin(2.0 * PI / Tb * millis() / 1000.0));

  // Watchdog
  if (tsli() >= KPD_TIMEOUT) {
    resetInputStr();
    cPtr = 0;
  }

  loops++; // increment loop counter

#ifdef DEBUGGING
  // serial debugger for LED strips
  if (tslp() >= DEBUG_PUSH_RATE) {
    Serial.print(F("Loops: "));
    Serial.print(1000UL * loops / DEBUG_PUSH_RATE);
    Serial.print(F("/s, Scans: "));
    Serial.print(scans);
    Serial.print(F("/s, Buffer: \""));
    Serial.print(inputStr);
    Serial.print(F("\","));
    Serial.println();
    // reset variables
    loops = 0;
    scans = 0;

    set_tolp();
  }
#endif
}
