#include <Arduino.h>
#include <SPI.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

// ==================================================
// --- DEFINIÇÕES: INFRAVERMELHO
// ==================================================
#define PINO_IR_RX 27
#define PINO_IR_TX 26

IRrecv irrecv(PINO_IR_RX);
IRsend irsend(PINO_IR_TX);
decode_results resultadosIR;

uint64_t codigoSalvoIR = 0;
decode_type_t protocoloSalvoIR = UNKNOWN;
uint16_t tamanhoBitsSalvoIR = 0;
bool temSinalIRClonado = false;

// ==================================================
// --- DEFINIÇÕES: RADIOFREQUÊNCIA (RAW)
// ==================================================
#define PINO_RF_CS 5
#define PINO_RF_GDO0 4

#define MAX_RAW_PULSOS 250
unsigned int bufferRawRF[MAX_RAW_PULSOS];
int tamanhoSinalRFClonado = 0;
bool temSinalRFClonado = false;

// ==================================================
// --- SETUP INICIAL
// ==================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==================================================");
  Serial.println("  FLIPPER DIY: SISTEMA DE CLONAGEM (IR & RF RAW)  ");
  Serial.println("==================================================");

  // 1. INICIALIZAÇÃO DO RÁDIO CC1101
  Serial.print("[CC1101] Inicializando SPI RF... ");
  ELECHOUSE_cc1101.setSpiPin(18, 19, 23, PINO_RF_CS);
  if (ELECHOUSE_cc1101.getCC1101()) {
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setModulation(2); // ASK/OOK
    ELECHOUSE_cc1101.setMHZ(433.92);   
    ELECHOUSE_cc1101.SetRx();          
    
    // Configuração simples do pino para leitura crua (RAW)
    pinMode(PINO_RF_GDO0, INPUT); 
    
    Serial.println("✅ SINAL VERDE (Modo Leitura RAW Ativado)");
  } else {
    Serial.println("❌ FALHA DE HARDWARE (CC1101 Nao Encontrado)");
  }
  
  // 2. INICIALIZAÇÃO DO INFRAVERMELHO
  Serial.print("[IR RECV] Ativando sensor no pino 27... ");
  irrecv.enableIRIn(); 
  Serial.println("✅ SINAL VERDE (Escutando sinais IR)");

  Serial.print("[IR SEND] Ativando emissor no pino 26... ");
  irsend.begin(); 
  Serial.println("✅ SINAL VERDE (Pronto para emitir IR)");

  Serial.println("==================================================");
  Serial.println("🛠️ STATUS: Aguardando comandos...");
  Serial.println("-> Aponte o controle da TV (IR clona automaticamente)");
  Serial.println("-> Digite 'C' para CLONAR campainha (Aproxime bem!)");
  Serial.println("-> Digite 'T' para disparar INFRAVERMELHO (TV/Ar)");
  Serial.println("-> Digite 'R' para disparar RADIOFREQUENCIA (Portoes)");
  Serial.println("-> Digite 'M' para abrir o MONITOR DE SINAL (Teste RF)");
  Serial.println("==================================================");
}

// ==================================================
// --- ROTINAS DE CAPTURA
// ==================================================
void infravermelho() {
  if (resultadosIR.bits < 4 || resultadosIR.decode_type == UNKNOWN) {
    irrecv.resume();
    return; 
  }
  
  Serial.println("\n--- 📺 NOVO SINAL INFRAVERMELHO CLONADO ---");
  protocoloSalvoIR = resultadosIR.decode_type;
  codigoSalvoIR = resultadosIR.value;
  tamanhoBitsSalvoIR = resultadosIR.bits;
  
  Serial.print("Protocolo: "); Serial.println(typeToString(protocoloSalvoIR));
  Serial.print("Código: 0x"); Serial.println(codigoSalvoIR, HEX);
  Serial.print("Tamanho: "); Serial.print(tamanhoBitsSalvoIR); Serial.println(" bits");
  
  temSinalIRClonado = true;
  Serial.println("💾 Sinal IR gravado! Aperte 'T' no terminal para retransmitir.");
  irrecv.resume();
}

void clonarRawRF() {
  Serial.println("\n--- 📡 MODO DE CAPTURA RF (RAW SQUELCH) ---");
  Serial.println("Aperte e SEGURE o botao da campainha BEM PERTO da antena...");

  unsigned long inicioEspera = millis();
  // SQUELCH: Fica travado esperando o sinal romper o ruido de -60dBm
  while (ELECHOUSE_cc1101.getRssi() < -60) {
    if (millis() - inicioEspera > 8000) {
      Serial.println("⏳ Tempo esgotado (8s). Nenhum sinal forte recebido.");
      return;
    }
  }

  Serial.println("💥 EXPLOSAO DE ENERGIA DETECTADA! Gravando onda...");

  // Sincroniza o início da gravação com a próxima borda de subida (HIGH)
  while(digitalRead(PINO_RF_GDO0) == HIGH);
  while(digitalRead(PINO_RF_GDO0) == LOW);

  unsigned long inicioGravacao = micros();
  unsigned long ultimoTempo = inicioGravacao;
  int estadoAtual = HIGH;
  tamanhoSinalRFClonado = 0;

  // Grava as transições elétricas do ar por 60 milissegundos
  while(micros() - inicioGravacao < 60000 && tamanhoSinalRFClonado < MAX_RAW_PULSOS) {
    int leitura = digitalRead(PINO_RF_GDO0);
    if (leitura != estadoAtual) {
      unsigned long agora = micros();
      bufferRawRF[tamanhoSinalRFClonado] = agora - ultimoTempo;
      ultimoTempo = agora;
      estadoAtual = leitura;
      tamanhoSinalRFClonado++;
    }
  }

  if (tamanhoSinalRFClonado > 20) {
    temSinalRFClonado = true;
    Serial.print("✅ Onda gravada com sucesso! Transicoes: ");
    Serial.println(tamanhoSinalRFClonado);
    Serial.println("💾 Aperte 'R' no terminal para retransmitir a onda.");
  } else {
    Serial.println("⚠️ Falha na captura. Sinal muito curto ou ruido.");
  }
}

// ==================================================
// --- ROTINAS DE TRANSMISSÃO
// ==================================================
void transmitirRF() {
  if (!temSinalRFClonado) {
    Serial.println("\n❌ ERRO: Nenhuma onda RF foi clonada ainda!");
    return;
  }

  Serial.println("\n>>> 🚀 DISPARANDO ONDA BRUTA (RADIOFREQUENCIA 433MHz) <<<");
  
  // 1. Prepara o CC1101 para Transmissão
  ELECHOUSE_cc1101.SetTx(); 
  pinMode(PINO_RF_GDO0, OUTPUT);
  
  // 2. Dispara a onda gravada 8 vezes seguidas (Burst)
  for (int repeticao = 0; repeticao < 8; repeticao++) {
    int estado = HIGH; // Sempre começa em HIGH conforme sincronizamos
    
    for (int i = 0; i < tamanhoSinalRFClonado; i++) {
      digitalWrite(PINO_RF_GDO0, estado);
      delayMicroseconds(bufferRawRF[i]);
      estado = !estado; // Inverte o pino para o próximo pulso da onda
    }
    
    digitalWrite(PINO_RF_GDO0, LOW);
    delay(15); // Gap de silêncio obrigatório entre as repetições
  }
  
  // 3. Devolve tudo ao estado de Escuta (RX)
  ELECHOUSE_cc1101.SetRx();
  pinMode(PINO_RF_GDO0, INPUT);
  
  Serial.println("✅ Transmissao RF RAW concluida! A campainha tocou?");
  Serial.println("==================================================");
}

// ==================================================
// --- LOOP PRINCIPAL
// ==================================================
void loop() {
  // Escuta ativa de luz (IR)
  if (irrecv.decode(&resultadosIR)) {
    infravermelho();
  }

  // Tratamento de comandos do Teclado
  if (Serial.available() > 0) {
    char comando = Serial.read();

    if (comando == '\n' || comando == '\r') return;

    // --- COMANDO: CLONAR RF (C) ---
    if (comando == 'C' || comando == 'c') {
      clonarRawRF();
    }
    // --- COMANDO: TRANSMITIR IR (T) ---
    else if (comando == 'T' || comando == 't') {
      if (temSinalIRClonado) {
        Serial.println("\n>>> 🚀 DISPARANDO SINAL INFRAVERMELHO GRAVADO <<<");
        irrecv.disableIRIn();
        delay(50);
        irsend.send(protocoloSalvoIR, codigoSalvoIR, tamanhoBitsSalvoIR);
        delay(50);
        irrecv.enableIRIn();
        Serial.println("✅ Transmissao IR concluida!");
      } else {
        Serial.println("\n❌ ERRO: Nenhum sinal IR clonado ainda.");
      }
    } 
    // --- COMANDO: TRANSMITIR RF (R) ---
    else if (comando == 'R' || comando == 'r') {
      transmitirRF();
    } 
    // --- COMANDO: MONITOR DE SINAL BRUTO (M) ---
    else if (comando == 'M' || comando == 'm') {
      Serial.println("\n--- 📡 MEDIDOR DE ENERGIA RF (ATIVO POR 5 SEGUNDOS) ---");
      Serial.println("Aperte o botao da campainha e segure perto da antena AGORA!");
      
      unsigned long inicio = millis();
      while (millis() - inicio < 5000) {
        int forcaSinal = ELECHOUSE_cc1101.getRssi();
        Serial.print("Potencia da Onda (RSSI): ");
        Serial.print(forcaSinal);
        Serial.println(" dBm");
        delay(250);
      }
      Serial.println("--- FIM DO TESTE DE HARDWARE ---");
    }
    // --- COMANDO INVALIDO ---
    else {
      Serial.print("\n⚠️ Comando ignorado (");
      Serial.print(comando);
      Serial.println("). Comandos validos: C, T, R, M.");
    }
  }
}