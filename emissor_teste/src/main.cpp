#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

// Pino físico onde você vai ligar a perna de SINAL (OUT) do seu receptor preto
#define PIN_RECEPTOR_IR 19

IRrecv irrecv(PIN_RECEPTOR_IR);
decode_results results;

void setup() {
  Serial.begin(115200);
  
  // Ativa o receptor para começar a escutar o ambiente
  irrecv.enableIRIn(); 
  
  Serial.println("=========================================");
  Serial.println("   Teste Isolado: Receptor IR Ativo      ");
  Serial.println("=========================================");
}

void loop() {
  // Se o receptor capturar qualquer raio infravermelho de um controle...
  if (irrecv.decode(&results)) {
    
    Serial.println("\n[SINAL DETECTADO!]");
    
    // Mostra o código do botão em formato Hexadecimal (Ex: 0x1FE48B7)
    Serial.print("Código bruto (HEX): 0x");
    Serial.println(resultToHexidecimal(&results));
    
    // Prepara o sensor para a próxima leitura
    irrecv.resume(); 
  }
  delay(100);
}