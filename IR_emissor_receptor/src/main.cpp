#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

// Definição dos pinos
#define PIN_RECEPTOR_IR 19
#define PIN_EMISSOR_IR 18

// Inicializa o receptor e o emissor nas respectivas portas
IRrecv irrecv(PIN_RECEPTOR_IR);
IRsend irsend(PIN_EMISSOR_IR);

decode_results results;

// Variáveis globais para armazenar o sinal clonado na memória RAM do ESP32
uint64_t codigoSalvo = 0;
decode_type_t protocoloSalvo = UNKNOWN;
uint16_t tamanhoBitsSalvo = 0;

void setup() {
  Serial.begin(115200);
  
  irrecv.enableIRIn(); // Ativa o receptor
  irsend.begin();      // Ativa o emissor
  
  Serial.println("=========================================");
  Serial.println("       ROBÔ CLONADOR DE INFRAVERMELHO    ");
  Serial.println("=========================================");
  Serial.println("1. Aponte o controle e aperte um botao para CLONAR.");
  Serial.println("2. Digite 'G' no monitor serial para TRANSMITIR o sinal salvo.");
  Serial.println("-----------------------------------------");
}

void loop() {
  // CAMADA 1: Captura e Clonagem
  if (irrecv.decode(&results)) {
    // Ignora códigos de repetição (segurar botão) para não corromper a memória
    if (results.value != 0xFFFFFFFFFFFFFFFF && results.value != 0) {
      
      codigoSalvo = results.value;
      protocoloSalvo = results.decode_type;
      tamanhoBitsSalvo = results.bits;

      Serial.println("\n[SINAL CLONADO COM SUCESSO!]");
      Serial.print("-> Protocolo: "); Serial.println(typeToString(protocoloSalvo));
      Serial.print("-> Codigo (HEX): 0x"); Serial.println(resultToHexidecimal(&results));
      Serial.print("-> Tamanho: "); Serial.print(tamanhoBitsSalvo); Serial.println(" bits");
      Serial.println("Status: Pronto para retransmitir. Envie 'G' no terminal para disparar.");
    }
    irrecv.resume(); // Continua escutando
  }

  // CAMADA 2: Transmissão via comando do Usuário
  if (Serial.available() > 0) {
    char comando = Serial.read();
    
    // Se o usuário digitou 'G' ou 'g' no terminal
    if (comando == 'G' || comando == 'g') {
      if (codigoSalvo != 0) {
        Serial.println("\n[TX] DISPARANDO SINAL INFRAVERMELHO...");
        
        // A biblioteca precisa que pare o receptor antes de transmitir para não dar eco
        irrecv.disableIRIn(); 
        
        // Envia o sinal armazenado baseando-se no protocolo correto clonado
        irsend.send(protocoloSalvo, codigoSalvo, tamanhoBitsSalvo);
        
        Serial.println("[TX] Sinal transmitido! Reativando modo escuta...");
        
        // Reativa o receptor
        irrecv.enableIRIn(); 
      } else {
        Serial.println("\n[AVISO] Nenhum sinal foi clonado ainda! Aperte um botao no controle primeiro.");
      }
    }
  }
  delay(50);
}