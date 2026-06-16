#include <Arduino.h>

// Definimos o pino onde o LED IR está conectado
#define PIN_LED_IR 18
#define LED_INTERNO 2

void setup() {
  Serial.begin(115200);
  
  // Configura os pinos como saída
  pinMode(PIN_LED_IR, OUTPUT);
  pinMode(LED_INTERNO, OUTPUT);
  
  Serial.println("=========================================");
  Serial.println("   Teste Isolado do LED Emissor IR       ");
  Serial.println("=========================================");
}

void loop() {
  // Liga o LED IR e o LED azul interno ao mesmo tempo
  digitalWrite(PIN_LED_IR, HIGH);
  digitalWrite(LED_INTERNO, HIGH);
  Serial.println("[IR] LIGADO (Invisível a olho nu)");
  delay(1000); // Aguarda 1 segundo
  
  // Desliga ambos
  digitalWrite(PIN_LED_IR, LOW);
  digitalWrite(LED_INTERNO, LOW);
  Serial.println("[IR] DESLIGADO");
  delay(1000); // Aguarda 1 segundo
}