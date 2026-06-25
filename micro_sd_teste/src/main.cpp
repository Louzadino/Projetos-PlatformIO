#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// Definimos o pino CS do cartão SD (conforme tabela acima)
#define PIN_SD_CS 5

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=========================================");
  Serial.println("      TESTE DE LEITURA DO CARTÃO SD      ");
  Serial.println("=========================================");

  // Inicializa o módulo SD no pino CS correto
  if (!SD.begin(PIN_SD_CS)) {
    Serial.println("❌ Falha ao inicializar o cartão SD!");
    Serial.println("Verifique as conexões físicas e se o cartão está inserido.");
    return;
  }
  
  Serial.println("✅ Cartão SD inicializado com sucesso!");

  // --- EXEMPLO DE GRAVAÇÃO DE ARQUIVO ---
  Serial.println("\nTentando criar o arquivo 'clones.txt'...");
  
  // Abre o arquivo para escrita (se não existir, ele cria automaticamente)
  File meuArquivo = SD.open("/clones.txt", FILE_WRITE);

  if (meuArquivo) {
    Serial.println("Gravando dados no arquivo...");
    
    // Escreve os dados simulando a formatação do projeto de vocês
    meuArquivo.println("--- CLONE INFRAVERMELHO LAB ---");
    meuArquivo.println("Protocolo: NEC");
    meuArquivo.println("Codigo: 0xFF6897");
    meuArquivo.println("--------------------------------");
    
    // Fecha o arquivo (obrigatório para salvar de verdade)
    meuArquivo.close();
    Serial.println("✅ Dados gravados e arquivo fechado!");
  } else {
    Serial.println("❌ Erro ao abrir o arquivo para escrita.");
  }

  // --- EXEMPLO DE LEITURA DE ARQUIVO ---
  Serial.println("\nLendo o conteúdo do arquivo 'clones.txt':");
  meuArquivo = SD.open("/clones.txt");
  if (meuArquivo) {
    // Enquanto houver caracteres no arquivo, exibe no monitor serial
    while (meuArquivo.available()) {
      Serial.write(meuArquivo.read());
    }
    meuArquivo.close();
  } else {
    Serial.println("❌ Erro ao abrir o arquivo para leitura.");
  }
}

void loop() {
  // Nada a fazer no loop para este teste isolado
}