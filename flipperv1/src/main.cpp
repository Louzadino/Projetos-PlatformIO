#include <Arduino.h>
#include <SPI.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

// --- DEFINIÇÃO DE PINOS ---
#define PINO_IR_RX 27
#define PINO_IR_TX 26

// Instância do Receptor e Emissor IR
IRrecv irrecv(PINO_IR_RX);
IRsend irsend(PINO_IR_TX);
decode_results resultadosIR;

// Variáveis Globais de Memória (Armazenam o último sinal lido)
uint64_t codigoSalvo = 0;
decode_type_t protocoloSalvo = UNKNOWN;
uint16_t tamanhoBitsSalvo = 0;
bool temSinalClonado = false;

// Configurações do Receptor RF (Mantidas para referência)
#define PINO_RF_CS 5
#define PINO_RF_GDO0 4
#define TEMPO_SILENCIO_MS 25      
#define MAX_PULSOS 400            
#define MIN_PULSOS_VALIDOS 45     

unsigned long tempoAnterior = 0;
unsigned long instanteUltimoPulso = 0;
uint16_t bufferDuracoes[MAX_PULSOS]; 
bool bufferEstados[MAX_PULSOS];
uint16_t contadorPulsos = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==================================================");
  Serial.println("  INICIALIZANDO SISTEMA MULTIPROTOCOLO (TESTE)    ");
  Serial.println("==================================================");

  // 2. TESTE DO RÁDIO CC1101 (Comentado/Inativo conforme pedido)
  Serial.print("[CC1101] Inicializando SPI RF... ");
  ELECHOUSE_cc1101.setSpiPin(18, 19, 23, PINO_RF_CS);
  if (ELECHOUSE_cc1101.getCC1101()) {
    Serial.println("✅ Comunicacao SPI: OK");
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setModulation(2); 
    ELECHOUSE_cc1101.setMHZ(433.92);   
    ELECHOUSE_cc1101.SetRx();          
    pinMode(PINO_RF_GDO0, INPUT);
    Serial.println("📡 Monitor filtrado ativo. Aperte o controle...");
    Serial.println("--------------------------------------------------");
  } else {
    Serial.println("❌ Erro de Hardware: CC1101 nao encontrado.");
  }
  
  // 3. INICIALIZAÇÃO DO RECEPTOR IR
  Serial.print("[IR RECV] Ativando sensor no pino 27... ");
  irrecv.enableIRIn(); 
  Serial.println("✅ SINAL VERDE (Escutando sinais IR)");

  // 4. INICIALIZAÇÃO DO TRANSMISSOR IR
  Serial.print("[IR SEND] Ativando emissor no pino 26... ");
  irsend.begin(); 
  Serial.println("✅ SINAL VERDE (Pronto para transmitir)");

  Serial.println("==================================================");
  Serial.println("🛠️ SISTEMA PRONTO. Aponte um controle remoto (TV/Ar)");
  Serial.println("para o sensor IR e aperte um botão para clonar.");
  Serial.println("-> Digite 'T' no Monitor Serial para retransmitir!");
  Serial.println("==================================================");
}

// Processa e armazena os dados do sinal recebido
void infravermelho() {
  // Ignora ruídos elétricos muito curtos no pino do IR
  if (resultadosIR.bits < 4 || resultadosIR.decode_type == UNKNOWN) {
    irrecv.resume();
    return; 
  }
  
  Serial.println("\n--- 📡 NOVO SINAL INFRAVERMELHO CLONADO ---");
  
  protocoloSalvo = resultadosIR.decode_type;
  codigoSalvo = resultadosIR.value;
  tamanhoBitsSalvo = resultadosIR.bits;
  
  // Mostra os dados detalhados na tela
  Serial.print("Protocolo Identificado: ");
  Serial.println(typeToString(protocoloSalvo));
  
  Serial.print("Código Hexadecimal (Ação): 0x");
  Serial.println(codigoSalvo, HEX);
  
  Serial.print("Tamanho: ");
  Serial.print(tamanhoBitsSalvo);
  Serial.println(" bits");
  
  temSinalClonado = true;
  Serial.println("💾 Sinal gravado na memoria! Pronto para retransmitir.");
  
  // Prepara o sensor para escutar o próximo sinal físico
  irrecv.resume();
}

// Função de leitura de RF (Mantida comentada conforme pedido)
void radiofrequencia() {
  int estadoAtual = digitalRead(PINO_RF_GDO0);
  static int estadoAnterior = LOW;
  unsigned long tempoAtual = micros();

  if (estadoAtual != estadoAnterior) {
    unsigned long duracao = tempoAtual - tempoAnterior;
    tempoAnterior = tempoAtual;
    instanteUltimoPulso = millis();

    if (duracao > 300 && duracao < 2500) {
      if (contadorPulsos < MAX_PULSOS) {
        bufferDuracoes[contadorPulsos] = duracao; 
        bufferEstados[contadorPulsos] = estadoAnterior; 
        contadorPulsos++;
      }
    }
    estadoAnterior = estadoAtual;
  }

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
    contadorPulsos = 0; 
  }
}

void loop() {
  // 1. Monitoramento do Receptor Infravermelho
  if (irrecv.decode(&resultadosIR)) {
    infravermelho();
  }

  // 2. Escuta ativa da Porta Serial (Teclado)
  if (Serial.available() > 0) {
    char comando = Serial.read();

    // Descarta quebras de linha enviadas automaticamente pelo Monitor Serial
    if (comando == '\n' || comando == '\r') {
      return;
    }

    // Se o caractere digitado for 'T' ou 't'
    if (comando == 'T' || comando == 't') {
      if (temSinalClonado) {
        Serial.println("\n>>> 🚀 DISPARANDO SINAL INFRAVERMELHO GRAVADO <<<");
        Serial.print("Enviando Protocolo: "); Serial.println(typeToString(protocoloSalvo));
        Serial.print("Enviando Codigo Hex: 0x"); Serial.println(codigoSalvo, HEX);
        Serial.print("Tamanho: "); Serial.print(tamanhoBitsSalvo); Serial.println(" bits");
        Serial.println("--------------------------------------------------");

        // Desativa o receptor IR temporariamente para que ele nao escute a propria transmissao
        irrecv.disableIRIn();
        delay(50);
        
        // Transmite de forma dinamica usando a definicao correta do protocolo clonado
        irsend.send(protocoloSalvo, codigoSalvo, tamanhoBitsSalvo);
        delay(50);
        
        // Reativa o receptor IR para voltar a escutar novos comandos
        irrecv.enableIRIn();
        
        Serial.println("✅ Transmissao concluida! Modo escuta reativado.");
        Serial.println("==================================================");
      } else {
        Serial.println("\n❌ ERRO: Nenhum sinal foi clonado ainda! Aperte um botao no controle primeiro.");
      }
    } else {
      Serial.print("\n⚠️ Comando ignorado (");
      Serial.print(comando);
      Serial.println("). Envie apenas 'T' para disparar o sinal.");
    }
  }

  // radiofrequencia(); // Mantido comentado para evitar conflitos
}