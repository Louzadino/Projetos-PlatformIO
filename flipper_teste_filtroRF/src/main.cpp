#include <Arduino.h>
#include <SPI.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <WiFi.h>
#include <WebServer.h>

void clonarSinal();
void transmitirIR();
void transmitirRF();

// ==================================================
// --- DEFINIÇÕES: WI-FI E SERVIDOR WEB
// ==================================================
const char* ssid = "FlipperZero_AP";  // NOME DA REDE QUE A ESP VAI CRIAR
const char* password = "12345678";    // SENHA DA REDE (mínimo 8 caracteres)

WebServer server(80);

void enviarCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

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
  Serial.println("FLIPPER DIY: SISTEMA DE CLONAGEM (IR & RF RAW)");
  Serial.println("==================================================\n");

  // 0. INICIALIZAÇÃO DO WI-FI (MODO AP)
  Serial.print("[WI-FI] Criando rede (AP): ");
  Serial.println(ssid);
  WiFi.softAP(ssid, password);
  
  Serial.println("\n✅ SINAL VERDE (Rede Wi-Fi Criada!)");
  Serial.print("🌐 CONECTE-SE AO WI-FI E ACESSE O IP: ");
  Serial.println(WiFi.softAPIP());

  // Configuração das rotas do Servidor Web
  server.on("/status", HTTP_OPTIONS, []() {
    enviarCORS();
    server.send(204);
  });
  
  server.on("/status", HTTP_GET, []() {
    enviarCORS();
    Serial.println("\n[HTTP] Novo dispositivo conectado a interface web!");
    server.send(200, "application/json", "{\"status\":\"ok\", \"message\":\"ESP32 Conectada!\"}");
  });

  server.on("/sniff", HTTP_OPTIONS, []() {
    enviarCORS();
    server.send(204);
  });

  server.on("/sniff", HTTP_GET, []() {
    enviarCORS();
    
    // Zera os estados anteriores
    temSinalRFClonado = false;
    temSinalIRClonado = false;
    irrecv.resume(); 
    
    // Tenta capturar algum sinal (IR ou RF)
    clonarSinal();
    
    String json = "{";

    if (temSinalRFClonado) {
      json += "\"status\":\"success\",";
      json += "\"type\":\"RF 433MHz\",";
      json += "\"hex\":\"SINAL RAW NA MEMORIA\",";
      json += "\"bits\":" + String(tamanhoSinalRFClonado);
    } 
    else if (temSinalIRClonado) {
      json += "\"status\":\"success\",";
      json += "\"type\":\"" + typeToString(protocoloSalvoIR) + "\",";
      json += "\"proto_id\":" + String(protocoloSalvoIR) + ",";
      json += "\"hex\":\"" + resultToHexidecimal(&resultadosIR) + "\",";
      json += "\"bits\":" + String(tamanhoBitsSalvoIR);
    } 
    else {
      json += "\"status\":\"timeout\",";
      json += "\"message\":\"Nenhum sinal detectado\"";
    }

    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/transmit", HTTP_OPTIONS, []() {
    enviarCORS();
    server.send(204);
  });

  server.on("/transmit", HTTP_POST, []() {
    enviarCORS();

    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"status\":\"error\"}");
      return;
    }
    
    String body = server.arg("plain");
    Serial.println("\n💻 [HTTP] Pedido de Disparo (TX): " + body);
    
    if (body.indexOf("\"method\":\"rf\"") != -1) {
      if (temSinalRFClonado) {
        transmitirRF();
        server.send(200, "application/json", "{\"status\":\"ok\"}");
      } 
      else {
        server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"RF na memoria foi perdido. Capture novamente.\"}");
      }
    } 
    else if (body.indexOf("\"method\":\"ir\"") != -1) {
      int hexIndex = body.indexOf("\"hex\":\"");
      int bitsIndex = body.indexOf("\"bits\":");
      int protoIdIndex = body.indexOf("\"proto_id\":");
      
      if (hexIndex != -1 && bitsIndex != -1 && protoIdIndex != -1) {
        int hexEnd = body.indexOf("\"", hexIndex + 7);
        String hexStr = body.substring(hexIndex + 7, hexEnd);
        
        int bitsEnd = body.indexOf(",", bitsIndex);
        if (bitsEnd == -1) bitsEnd = body.indexOf("}", bitsIndex);
        String bitsStr = body.substring(bitsIndex + 7, bitsEnd);
        
        int protoEnd = body.indexOf(",", protoIdIndex);
        if (protoEnd == -1) protoEnd = body.indexOf("}", protoIdIndex);
        String protoStr = body.substring(protoIdIndex + 11, protoEnd);
        
        codigoSalvoIR = strtoull(hexStr.c_str(), NULL, 16);
        tamanhoBitsSalvoIR = bitsStr.toInt();
        protocoloSalvoIR = (decode_type_t)protoStr.toInt();
        temSinalIRClonado = true;
      }
      
      if (temSinalIRClonado) {
        Serial.println(">>> 🚀 DISPARANDO SINAL INFRAVERMELHO (WEB) <<<");
        transmitirIR();
        server.send(200, "application/json", "{\"status\":\"ok\"}");
      } 
      else {
        server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Nenhum IR válido recebido.\"}");
      }
    }
  });

  server.begin();
  Serial.println("🌐 Servidor HTTP iniciado.");

  // 1. INICIALIZAÇÃO DO RÁDIO CC1101
  Serial.println("[CC1101] Inicializando SPI RF... ");
  
  // INICIA O BARRAMENTO ANTES DA BIBLIOTECA
  ELECHOUSE_cc1101.setSpiPin(18, 19, 23, PINO_RF_CS);

  if (ELECHOUSE_cc1101.getCC1101()) {
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setModulation(2); // OOK
    ELECHOUSE_cc1101.setMHZ(433.92);   
    ELECHOUSE_cc1101.SetRx();          
    
    // Configuração simples do pino para leitura crua (RAW)
    pinMode(PINO_RF_GDO0, INPUT); 
    
    Serial.println("✅ SINAL VERDE (Modo Leitura RAW Ativado)");
  } 
  else {
    Serial.println("❌ FALHA DE HARDWARE (CC1101 Nao Encontrado)");
  }
  
  // 2. INICIALIZAÇÃO DO INFRAVERMELHO
  Serial.println("[IR RECV] Ativando sensor no pino 27...");
  irrecv.enableIRIn(); 
  Serial.println("✅ SINAL VERDE (Escutando sinais IR)");

  Serial.println("[IR SEND] Ativando emissor no pino 26... ");
  irsend.begin(); 
  Serial.println("✅ SINAL VERDE (Pronto para emitir IR)");
}

// ==================================================
// --- ROTINAS DE CAPTURA
// ==================================================
void capturaIR() {
  if (resultadosIR.bits < 4 || resultadosIR.decode_type == UNKNOWN) {
    irrecv.resume();
    return; 
  }
  
  Serial.println("\n--- 📺 NOVO SINAL INFRAVERMELHO CLONADO ---");
  protocoloSalvoIR = resultadosIR.decode_type;
  codigoSalvoIR = resultadosIR.value;
  tamanhoBitsSalvoIR = resultadosIR.bits;
  
  Serial.print("Protocolo: "); Serial.println(typeToString(protocoloSalvoIR));
  Serial.print("Código: "); Serial.println(codigoSalvoIR, HEX);
  Serial.print("Tamanho: "); Serial.print(tamanhoBitsSalvoIR); Serial.println(" bits");
  
  temSinalIRClonado = true;
  irrecv.resume();
}

void capturaRF() {
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
  } 
  else {
    Serial.println("❌ Falha na captura. Sinal muito curto ou ruido.");
  }
}

void clonarSinal() {
  Serial.println("\n---  MODO DE CAPTURA DE SINAL ATIVO COM FILTRO PARA RF (RAW SQUELCH) ---");
  unsigned long inicioEspera = millis();

  // SQUELCH: Fica travado esperando o sinal romper o ruido de -60dBm ou receber um IR
  while (ELECHOUSE_cc1101.getRssi() < -60) {

    if (millis() - inicioEspera > 8000) {
      Serial.println("⏳ Tempo esgotado (8s). Nenhum sinal forte recebido.");
      return;
    }

    // Permite que o IR interrompa a escuta do RF instantaneamente
    if (irrecv.decode(&resultadosIR)) {
      capturaIR();

      if (temSinalIRClonado) {
        Serial.println("📺 Sinal IR detectado!");
        return;
      }
    }
  }

  Serial.println("💥 EXPLOSAO DE ENERGIA DETECTADA! Gravando onda...");
  capturaRF();
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

void transmitirIR() {
  irrecv.disableIRIn();
  delay(50);
  irsend.send(protocoloSalvoIR, codigoSalvoIR, tamanhoBitsSalvoIR);
  delay(50);
  irrecv.enableIRIn();
  Serial.println("✅ Transmissao IR (Web) concluida!");
}

// ==================================================
// --- LOOP PRINCIPAL
// ==================================================
void loop() {
  // Trata requisições HTTP recebidas do navegador
  server.handleClient();
}