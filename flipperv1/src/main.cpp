#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

// --- DEFINIÇÃO DE PINOS ---
// Barramento SPI Compartilhado: 18 (SCK), 19 (MISO), 23 (MOSI)
#define PINO_RF_CS 5
#define PINO_RF_GDO0 4
#define PINO_SD_CS 15     // <-- Novo pino do Cartão SD!
#define PINO_IR_RX 27
#define PINO_IR_TX 26

// Instância do Receptor IR
IRrecv irrecv(PINO_IR_RX, 1024, 15, true);
decode_results resultadosIR;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==================================================");
  Serial.println("  INICIALIZANDO SISTEMA MULTIPROTOCOLO (TESTE)    ");
  Serial.println("==================================================");

  // 1. TESTE DO CARTÃO SD (CS = 15)
  Serial.print("[SD CARD] Inicializando... ");
  // O SD falhar não deve travar o resto do sistema
  if (!SD.begin(PINO_SD_CS)) {
    Serial.println("❌ FALHA (Aguardando cartao de menor capacidade)");
  } else {
    Serial.println("✅ SINAL VERDE (Pronto para gravar logs)");
  }

  // 2. TESTE DO RÁDIO CC1101 (CS = 5)
  /*
  Serial.print("[CC1101] Inicializando SPI RF... ");
  ELECHOUSE_cc1101.setSpiPin(18, 19, 23, PINO_RF_CS);
  if (ELECHOUSE_cc1101.getCC1101()) {
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setMHZ(433.92);
    Serial.println("✅ SINAL VERDE (Rádio Online)");
  } else {
    Serial.println("❌ FALHA DE COMUNICACAO (Verifique os fios!)");
  }
  */
  
  // 3. TESTE DO INFRAVERMELHO (IR RX)
  Serial.print("[IR RECV] Ativando sensor no pino 27... ");
  irrecv.enableIRIn(); // Inicia o receptor
  Serial.println("✅ SINAL VERDE (Escutando sinais IR)");

  // 4. PREPARAÇÃO DO TRANSMISSOR IR
  pinMode(PINO_IR_TX, OUTPUT);
  digitalWrite(PINO_IR_TX, LOW);
  Serial.println("[IR SEND] Transmissor configurado no pino 26.");

  Serial.println("==================================================");
  Serial.println("🛠️ SISTEMA PRONTO. Aponte um controle remoto (TV/Ar)");
  Serial.println("para o sensor IR e aperte um botão para testar.");
  Serial.println("==================================================");
}

void loop() {
  // Verifica se o sensor IR capturou algum pacote de luz invisível no ar
  if (irrecv.decode(&resultadosIR)) {
    Serial.println("\n--- NOVO SINAL INFRAVERMELHO DETECTADO ---");
    
    // A biblioteca processa os pulsos e diz de qual marca é a TV/Ar (NEC, Samsung, Sony, etc)
    Serial.print("Protocolo Identificado: ");
    Serial.println(typeToString(resultadosIR.decode_type));
    
    // Mostra o código hexadecimal do botão que foi apertado
    Serial.print("Código Hexadecimal (Ação): 0x");
    Serial.println(resultadosIR.value, HEX);
    
    // Mostra quantos bits a senha tem
    Serial.print("Tamanho: ");
    Serial.print(resultadosIR.bits);
    Serial.println(" bits");

    // Prepara para escutar o próximo botão
    irrecv.resume();
  }
}