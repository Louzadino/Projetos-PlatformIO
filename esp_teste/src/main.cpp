#include <Arduino.h>

#define LED_PIN 2

void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("ESP32 Ativo e Operante!");
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(1000);
}