/*
 * ============================================================================
 * PROJETO FINAL IoT - UNIFESP ICT SJC
 * PLACA 2: Firmware do Nó Principal (Gateway LoRa 915MHz RX + OLED + MQTT)
 * Arquivo: 03-firmware/no-principal-gateway/no-principal-gateway.ino
 * Placa na IDE: Heltec WiFi LoRa 32(V3)
 * ============================================================================
 */

#include "LoRaWan_APP.h"
#include <Arduino.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <WiFi.h>

// ============================================================================
// 1. CONFIGURAÇÕES DE WI-FI (Opcional para teste de bancada LoRa)
// ============================================================================
#define HABILITAR_WIFI false  // Mude para true quando quiser conectar ao Wi-Fi

const char* WIFI_SSID = "SUA_REDE_WIFI";
const char* WIFI_PASS = "SUA_SENHA_WIFI";

// ============================================================================
// 2. PARÂMETROS RF LoRa PADRONIZADOS (Idênticos ao Nó Medidor)
// ============================================================================
#define RF_FREQUENCY               915000000 // 915 MHz (ANATEL Brasil)
#define LORA_BANDWIDTH             0         // 0 = 125 kHz
#define LORA_SPREADING_FACTOR      7         // SF7
#define LORA_CODINGRATE            1         // 1 = 4/5
#define LORA_PREAMBLE_LEN          8         // 8 símbolos
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false

// ============================================================================
// OBJETOS E VARIÁVEIS GLOBAIS
// ============================================================================
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
static RadioEvents_t RadioEvents;

bool loraIdle = true;
bool pacoteNovoParaProcessar = false;

// Buffer de 512 bytes protegido contra estouro de pilha (Stack Smashing)
char rxPayload[512];
int16_t ultimoRSSI = 0;
int8_t  ultimoSNR  = 0;
unsigned long pacotesValidos = 0;

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

// Extrai valores do JSON sem depender de bibliotecas externas
String extrairCampoJSON(const String &json, const String &chave) {
  String busca = "\"" + chave + "\":";
  int idx = json.indexOf(busca);
  if (idx == -1) return "--";
  idx += busca.length();

  while (idx < (int)json.length() && json[idx] == ' ') idx++;

  if (json[idx] == '\"') {
    idx++;
    int fim = json.indexOf('\"', idx);
    return (fim != -1) ? json.substring(idx, fim) : "--";
  } else {
    int fimVirgula = json.indexOf(',', idx);
    int fimChave   = json.indexOf('}', idx);
    int fim = (fimVirgula != -1 && fimVirgula < fimChave) ? fimVirgula : fimChave;
    return (fim != -1) ? json.substring(idx, fim) : "--";
  }
}

// Callback seguro de recepção LoRa
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  uint16_t tamCopiar = (size < sizeof(rxPayload) - 1) ? size : (sizeof(rxPayload) - 1);
  memcpy(rxPayload, payload, tamCopiar);
  rxPayload[tamCopiar] = '\0';

  ultimoRSSI = rssi;
  ultimoSNR  = snr;
  pacoteNovoParaProcessar = true;

  Radio.Sleep();
  loraIdle = true;
}

void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  VextON();
  delay(100);
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.clear();
  display.drawString(0, 0, "UNIFESP - GATEWAY LORA");
  display.drawString(0, 16, "Iniciando Radio 915MHz...");
  display.display();

  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LEN,
                    0, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  if (HABILITAR_WIFI) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    unsigned long inicio = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - inicio < 4000)) {
      delay(250);
    }
  }

  display.clear();
  display.drawString(0, 0, "GATEWAY LORA PRONTO");
  display.drawString(0, 16, "Escutando 915 MHz (SF7)");
  display.drawString(0, 32, "Filtro: node_id ativo");
  display.drawString(0, 48, "Aguardando No Medidor...");
  display.display();

  Serial.println("\n=======================================================");
  Serial.println(" GATEWAY LORA 915MHz INICIADO - AGUARDANDO PACOTES...  ");
  Serial.println("=======================================================");
}

void loop() {
  if (loraIdle) {
    loraIdle = false;
    Radio.Rx(0);
  }
  Radio.IrqProcess();

  if (pacoteNovoParaProcessar) {
    pacoteNovoParaProcessar = false;
    String jsonStr = String(rxPayload);

    // Filtra apenas pacotes que pertencem ao nosso projeto (que possuem "node_id")
    if (jsonStr.indexOf("\"node_id\"") == -1) {
      Serial.printf("[RX Ignorado - Outro Grupo] (%d dBm): %s\n", ultimoRSSI, rxPayload);
      return;
    }

    pacotesValidos++;
    String nodeId = extrairCampoJSON(jsonStr, "node_id");
    String seq    = extrairCampoJSON(jsonStr, "seq");
    String corrA  = extrairCampoJSON(jsonStr, "corrente_a");
    String potW   = extrairCampoJSON(jsonStr, "potencia_w");
    String kwh    = extrairCampoJSON(jsonStr, "energia_kwh");
    String luz    = extrairCampoJSON(jsonStr, "luz_acesa");

    // 1. Imprime os dados recebidos no Monitor Serial
    Serial.println("-------------------------------------------------------");
    Serial.printf("[LoRa RX #%lu] RSSI: %d dBm | SNR: %d dB\n", pacotesValidos, ultimoRSSI, ultimoSNR);
    Serial.printf("JSON: %s\n", rxPayload);
    Serial.printf("Resumo -> No: %s (#%s) | I: %s A | P: %s W | E: %s kWh | Luz: %s\n",
                  nodeId.c_str(), seq.c_str(), corrA.c_str(), potW.c_str(), kwh.c_str(),
                  (luz == "true") ? "ACESA" : "APAGADA");

    // 2. Atualiza o Display OLED do Gateway
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "RX: " + nodeId + " (#" + seq + ")");
    display.drawString(0, 13, "Pot: " + potW + "W | I: " + corrA + "A");
    display.drawString(0, 26, "E: " + kwh + "kWh | Luz:" + String((luz == "true") ? "ON" : "OFF"));
    display.drawString(0, 39, "RSSI:" + String(ultimoRSSI) + "dBm | SNR:" + String(ultimoSNR) + "dB");
    display.drawString(0, 52, "Pacotes Validos: " + String(pacotesValidos));
    display.display();
  }
}