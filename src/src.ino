#include <Arduino.h>
const int LED_PIN = 13;

void setup() {
    Serial.begin(9600);
    while (!Serial)
        ;
    Serial.println("Start blinking...");
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("on");
    delay(500);
    digitalWrite(LED_PIN, LOW);
    Serial.println("off");
    delay(500);
}
