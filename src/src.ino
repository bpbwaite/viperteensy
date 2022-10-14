#include <Arduino.h>
#include <Keypad.h>
#include <SoftwareSerial.h>

#include <Adafruit_NeoPixel.h>
#include <Adafruit_Soundboard.h>

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                         {'4', '5', '6', 'B'},
                         {'7', '8', '9', 'C'},
                         {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {5, 4, 3, 2}; // connect to the row pinouts of the kpd
byte colPins[COLS] = {9, 8, 7, 6}; // connect to the column pinouts of the kpd

Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

unsigned long loopCount;
unsigned long startTime;
String msg;

void setup() {
  Serial.begin(9600);
  loopCount = 0;
  startTime = millis();
  msg = "";
  kpd.setDebounceTime(10);
}

void loop() {
  loopCount++;
  if ((millis() - startTime) > 5000) {
    Serial.print("Average loops per second = ");
    Serial.println(loopCount / 5);
    startTime = millis();
    loopCount = 0;
  }

  // Fills kpd.key[ ] array with up-to 10 active keys.
  // Returns true if there are ANY active keys.
  if (kpd.getKeys()) {
    for (int i = 0; i < LIST_MAX; i++) // Scan the whole key list.
    {
      if (kpd.key[i].stateChanged) // Only find keys that have changed state.
      {
        switch (kpd.key[i].kstate) { // Report active key state : IDLE, PRESSED,
                                     // HOLD, or RELEASED
        case PRESSED:
          msg = " PRESSED.";
          break;
        case HOLD:
          msg = " HOLD.";
          break;
        case RELEASED:
          msg = " RELEASED.";
          break;
        case IDLE:
          msg = " IDLE.";
        }
        Serial.print("Key ");
        Serial.print(kpd.key[i].kchar);
        Serial.println(msg);
      }
    }
  }
} // End loop
/*
#define SFX_RST 9

Adafruit_NeoPixel strip = Adafruit_NeoPixel(144 * 4, 19, NEO_GRB + NEO_KHZ800);

Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, SFX_RST);
*/
/************************ MENU HELPERS ***************************/
/*
void flushInput() {
  // Read all available serial input to flush pending data.
  uint16_t timeoutloop = 0;
  while (timeoutloop++ < 40) {
    while (Serial1.available()) {
      Serial1.read();
      timeoutloop = 0; // If char was received reset the timer
    }
    delay(1);
  }
}

char readBlocking() {
  while (!Serial.available())
    ;
  return Serial.read();
}

uint16_t readnumber() {
  uint16_t x = 0;
  char c;
  while (!isdigit(c = readBlocking())) {
    // Serial.print(c);
  }
  Serial.print(c);
  x = c - '0';
  while (isdigit(c = readBlocking())) {
    Serial.print(c);
    x *= 10;
    x += c - '0';
  }
  return x;
}

uint8_t readline(char *buff, uint8_t maxbuff) {
  uint16_t buffidx = 0;

  while (true) {
    if (buffidx > maxbuff) {
      break;
    }

    if (Serial.available()) {
      char c = Serial.read();
      // Serial.print(c, HEX); Serial.print("#"); Serial.println(c);

      if (c == '\r')
        continue;
      if (c == 0xA) {
        if (buffidx == 0) { // the first 0x0A is ignored
          continue;
        }
        buff[buffidx] = 0; // null term
        return buffidx;
      }
      buff[buffidx] = c;
      buffidx++;
    }
  }
  buff[buffidx] = 0; // null term
  return buffidx;
}

void setup() {
  // put your setup code here, to run once:
  strip.setBrightness(10);
  strip.begin();
  strip.rainbow(0, 20);
  strip.show();

  Serial.begin(9600);
  while (!Serial)
    ;
  Serial1.begin(9600);
  if (!sfx.reset()) {
    Serial.println("Not found");
    while (1)
      ;
  };
}

void loop() {
  flushInput();

  Serial.println(F("What would you like to do?"));
  Serial.println(F("[r] - reset"));
  Serial.println(F("[+] - Vol +"));
  Serial.println(F("[-] - Vol -"));
  Serial.println(F("[L] - List files"));
  Serial.println(F("[P] - play by file name"));
  Serial.println(F("[#] - play by file number"));
  Serial.println(F("[=] - pause playing"));
  Serial.println(F("[>] - unpause playing"));
  Serial.println(F("[q] - stop playing"));
  Serial.println(F("[t] - playtime status"));
  Serial.println(F("> "));

  while (!Serial.available()) {
    strip.rainbow((100 * millis()) % 65535, 20);
    strip.show();
  }
  char cmd = Serial.read();

  flushInput();

  switch (cmd) {
  case 'r': {
    if (!sfx.reset()) {
      Serial.println("Reset failed");
    }
    break;
  }

  case 'L': {
    uint8_t files = sfx.listFiles();

    Serial.println("File Listing");
    Serial.println("========================");
    Serial.println();
    Serial.print("Found ");
    Serial.print(files);
    Serial.println(" Files");
    for (uint8_t f = 0; f < files; f++) {
      Serial.print(f);
      Serial.print("\tname: ");
      Serial.print(sfx.fileName(f));
      Serial.print("\tsize: ");
      Serial.println(sfx.fileSize(f));
    }
    Serial.println("========================");
    break;
  }

  case '#': {
    Serial.print("Enter track #");
    uint8_t n = readnumber();

    Serial.print("\nPlaying track #");
    Serial.println(n);
    if (!sfx.playTrack((uint8_t)n)) {
      Serial.println("Failed to play track?");
    }
    break;
  }

  case 'P': {
    Serial.print("Enter track name (full 12 character name!) >");
    char name[20];
    readline(name, 20);

    Serial.print("\nPlaying track \"");
    Serial.print(name);
    Serial.print("\"");
    if (!sfx.playTrack(name)) {
      Serial.println("Failed to play track?");
    }
    break;
  }

  case '+': {
    Serial.println("Vol up...");
    uint16_t v;
    if (!(v = sfx.volUp())) {
      Serial.println("Failed to adjust");
    } else {
      Serial.print("Volume: ");
      Serial.println(v);
    }
    break;
  }

  case '-': {
    Serial.println("Vol down...");
    uint16_t v;
    if (!(v = sfx.volDown())) {
      Serial.println("Failed to adjust");
    } else {
      Serial.print("Volume: ");
      Serial.println(v);
    }
    break;
  }

  case '=': {
    Serial.println("Pausing...");
    if (!sfx.pause())
      Serial.println("Failed to pause");
    break;
  }

  case '>': {
    Serial.println("Unpausing...");
    if (!sfx.unpause())
      Serial.println("Failed to unpause");
    break;
  }

  case 'q': {
    Serial.println("Stopping...");
    if (!sfx.stop())
      Serial.println("Failed to stop");
    break;
  }

  case 't': {
    Serial.print("Track time: ");
    uint32_t current, total;
    if (!sfx.trackTime(&current, &total))
      Serial.println("Failed to query");
    Serial.print(current);
    Serial.println(" seconds");
    break;
  }

  case 's': {
    Serial.print("Track size (bytes remaining/total): ");
    uint32_t remain, total;
    if (!sfx.trackSize(&remain, &total))
      Serial.println("Failed to query");
    Serial.print(remain);
    Serial.print("/");
    Serial.println(total);
    break;
  }
  }
}
*/
