#include <Arduino.h>
#include <SPI.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h>        // <-- Nova biblioteca para limpar o RF!
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
// --- DEFINIÇÕES: RADIOFREQUÊNCIA
// ==================================================
#define PINO_RF_CS 5
#define PINO_RF_GDO0 4

RCSwitch mySwitch = RCSwitch(); // Instância do decodificador de rádio

unsigned long codigoSalvoRF = 0;
unsigned int tamanhoBitsSalvoRF = 0;
int protocoloSalvoRF = 0;
bool temSinalRFClonado = false;

// ==================================================
// --- SETUP INICIAL
// ==================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==================================================");
  Serial.println("  FLIPPER DIY: SISTEMA DE CLONAGEM (IR & RF)      ");
  Serial.println("==================================================");

  // 1. INICIALIZAÇÃO DO RÁDIO CC1101
  Serial.print("[CC1101] Inicializando SPI RF... ");
  ELECHOUSE_cc1101.setSpiPin(18, 19, 23, PINO_RF_CS);
  if (ELECHOUSE_cc1101.getCC1101()) {
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setModulation(2); // ASK/OOK
    ELECHOUSE_cc1101.setMHZ(433.92);   
    ELECHOUSE_cc1101.SetRx();          
    
    // CORREÇÃO: Usa mapeamento oficial de interrupção do ESP32
    mySwitch.enableReceive(digitalPinToInterrupt(PINO_RF_GDO0)); 
    
    Serial.println("✅ SINAL VERDE (Filtro Anti-Ruido Ativado)");
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
  Serial.println("🛠️ STATUS: Aguardando captura limpa de sinais...");
  Serial.println("-> Aperte o controle da TV ou o botao da Campainha");
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

void radiofrequencia() {
  // O RCSwitch só entra neste 'if' se identificar um padrão real de controle
  if (mySwitch.available()) {
    Serial.println("\n--- 📡 NOVO SINAL RADIOFREQUENCIA (433MHz) CLONADO ---");
    
    codigoSalvoRF = mySwitch.getReceivedValue();
    tamanhoBitsSalvoRF = mySwitch.getReceivedBitlength();
    protocoloSalvoRF = mySwitch.getReceivedProtocol();
    
    // Ignora códigos zerados (ruídos anômalos que passaram pelo filtro)
    if (codigoSalvoRF != 0) {
      Serial.print("Código Decimal: "); Serial.println(codigoSalvoRF);
      Serial.print("Tamanho: "); Serial.print(tamanhoBitsSalvoRF); Serial.println(" bits");
      Serial.print("Protocolo RC: "); Serial.println(protocoloSalvoRF);
      
      temSinalRFClonado = true;
      Serial.println("💾 Sinal RF gravado! Aperte 'R' no terminal para tocar a campainha.");
    } else {
      Serial.println("⚠️ Sinal fraco ou corrompido detectado.");
    }

    // Limpa a memória do filtro para a próxima leitura
    mySwitch.resetAvailable();
  }
}

// ==================================================
// --- ROTINAS DE TRANSMISSÃO
// ==================================================
void transmitirRF() {
  if (!temSinalRFClonado) {
    Serial.println("\n❌ ERRO: Nenhum sinal RF foi clonado ainda!");
    return;
  }

  Serial.println("\n>>> 🚀 DISPARANDO SINAL RADIOFREQUENCIA (433MHz) <<<");
  Serial.print("Transmitindo Código: "); Serial.println(codigoSalvoRF);
  
  // 1. Prepara o CC1101 para Transmissão
  ELECHOUSE_cc1101.SetTx(); 
  mySwitch.disableReceive(); // Para de ouvir para não causar interferência
  
  // 2. Prepara o protocolo na porta GDO0 e transmite
  mySwitch.enableTransmit(PINO_RF_GDO0);
  mySwitch.setProtocol(protocoloSalvoRF);
  
  // O send já envia repetido várias vezes automaticamente
  mySwitch.send(codigoSalvoRF, tamanhoBitsSalvoRF); 
  
  // 3. Devolve tudo ao estado de Escuta (RX)
  mySwitch.disableTransmit();
  ELECHOUSE_cc1101.SetRx();
  mySwitch.enableReceive(PINO_RF_GDO0);
  
  Serial.println("✅ Transmissao de RF concluida! A campainha tocou?");
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

  // Escuta inteligente de rádio (RF via RCSwitch)
  radiofrequencia();

  // Tratamento de comandos do Teclado
  if (Serial.available() > 0) {
    char comando = Serial.read();

    if (comando == '\n' || comando == '\r') return;

    if (comando == 'T' || comando == 't') {
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
    else if (comando == 'R' || comando == 'r') {
      transmitirRF();
    } 
    // --- NOVO COMANDO: MONITOR DE SINAL BRUTO (RSSI) ---
    else if (comando == 'M' || comando == 'm') {
      Serial.println("\n--- 📡 MEDIDOR DE ENERGIA RF (ATIVO POR 5 SEGUNDOS) ---");
      Serial.println("Aperte o botao da campainha e segure perto da antena AGORA!");
      
      unsigned long inicio = millis();
      // Fica lendo a potência do sinal no ar por 5 segundos
      while (millis() - inicio < 5000) {
        int forcaSinal = ELECHOUSE_cc1101.getRssi();
        
        Serial.print("Potencia da Onda (RSSI): ");
        Serial.print(forcaSinal);
        Serial.println(" dBm");
        
        delay(250); // Leitura a cada 1/4 de segundo
      }
      Serial.println("--- FIM DO TESTE DE HARDWARE ---");
    }
  }
}