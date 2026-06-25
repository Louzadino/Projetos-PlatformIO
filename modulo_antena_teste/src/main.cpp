#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define PIN_CC1101_CS 5
#define PIN_CC1101_GDO0 4

#define TEMPO_SILENCIO_MS 25      
#define MAX_PULSOS 400            
#define MIN_PULSOS_VALIDOS 45     // <-- FILTRO AUMENTADO: Ignora pacotes curtos de ruído

unsigned long tempoAnterior = 0;
unsigned long instanteUltimoPulso = 0;
uint16_t bufferDuracoes[MAX_PULSOS]; 
bool bufferEstados[MAX_PULSOS];
uint16_t contadorPulsos = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("==================================================");
  Serial.println("   CC1101: SNIFFER COMPACTO (ANTI-RUIDO)         ");
  Serial.println("==================================================");

  ELECHOUSE_cc1101.setSpiPin(18, 19, 23, PIN_CC1101_CS);
  
  if (ELECHOUSE_cc1101.getCC1101()) {
    Serial.println("✅ Comunicacao SPI: OK");
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setModulation(2); 
    ELECHOUSE_cc1101.setMHZ(433.92);   
    ELECHOUSE_cc1101.SetRx();          
    pinMode(PIN_CC1101_GDO0, INPUT);
    Serial.println("📡 Monitor filtrado ativo. Aperte o controle...");
    Serial.println("--------------------------------------------------");
  } else {
    Serial.println("❌ Erro de Hardware: CC1101 nao encontrado.");
  }
}

void loop() {
  int estadoAtual = digitalRead(PIN_CC1101_GDO0);
  static int estadoAnterior = LOW;
  unsigned long tempoAtual = micros();

  if (estadoAtual != estadoAnterior) {
    unsigned long duracao = tempoAtual - tempoAnterior;
    tempoAnterior = tempoAtual;
    instanteUltimoPulso = millis();

    // <-- FILTRO DE LARGURA AJUSTADO: foca na faixa padrão de controles (300us a 2500us)
    if (duracao > 300 && duracao < 2500) {
      if (contadorPulsos < MAX_PULSOS) {
        bufferDuracoes[contadorPulsos] = duracao; 
        bufferEstados[contadorPulsos] = estadoAnterior; 
        contadorPulsos++;
      }
    }
    estadoAnterior = estadoAtual;
  }

  // Se houve silêncio no ar, processa o pacote
  if (contadorPulsos > 0 && (millis() - instanteUltimoPulso > TEMPO_SILENCIO_MS)) {
    if (contadorPulsos >= MIN_PULSOS_VALIDOS) {
      
      Serial.println();
      Serial.println("======= 📡 SINAL REAL CAPTURADO =======");
      Serial.print("📊 Transicoes: "); Serial.print(contadorPulsos); Serial.println(" pulsos");
      
      for (uint16_t i = 0; i < contadorPulsos; i++) {
        Serial.print(bufferEstados[i] == HIGH ? "+" : "-");
        Serial.print(bufferDuracoes[i]);
        Serial.print(" ");

        if ((i + 1) % 12 == 0) Serial.println();
      }
      Serial.println("\n==================================================");
    }
    contadorPulsos = 0; // Limpa o buffer de qualquer maneira para não acumular lixo
  }
}