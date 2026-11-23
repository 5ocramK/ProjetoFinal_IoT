/*
 * PINAGEM DOS SENSORES:
 * - GPIO 1 (ADC): Saida OUT do ACS712 (divisor 2x 10kΩ + cap 100nF)
 * - GPIO 2 (ADC): Divisor de tensao do LDR (3.3V -> LDR -> GPIO 2 -> 10kΩ -> GND)
 * - GPIO 3 (ADC): Pino central do Potenciometro (extremos em 3.3V e GND)
 */

#include "LoRaWan_APP.h"
#include <Arduino.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"

// --- Identificacao deste No Medidor ---
#define NODE_ID_PADRAO "comodo_1"
#define TIPO_PADRAO    "individual"
#define MORADOR_PADRAO "Morador 1"

// Se true, alterna o envio entre "comodo_1" (Potenciometro) e "area_comum" (ACS712 + LDR)
// para permitir demonstrar o rateio completo no Node-RED usando apenas 2 placas Heltec!
#define ALTERNAR_COMODOS_DEMO true

// --- Pinos Analogicos (ADC1 do Heltec V3) ---
#define PIN_ACS712  1   // GPIO 1: Sensor de corrente ACS712
#define PIN_LDR     2   // GPIO 2: Sensor de iluminacao LDR
#define PIN_POT     3   // GPIO 3: Potenciometro de variacao de carga

// --- Parametros Eletricos ---
#define TENSAO_REDE_V              127.0f
#define CORRENTE_MAX_POT_A         20.0f
#define POTENCIA_LAMPADA_W         60.0f
#define POTENCIA_BASE_COMUM_W      150.0f  // Geladeira/roteador na Area Comum
#define LIMIAR_LDR_MV              1500
#define SENSIBILIDADE_ACS_MV_POR_A 92.5f

// --- Parametros RF LoRa ---
#define RF_FREQUENCY               915000000 // 915 MHz (ANATEL Brasil)
#define TX_OUTPUT_POWER            14        // 14 dBm
#define LORA_BANDWIDTH             0         // 0 = 125 kHz
#define LORA_SPREADING_FACTOR      7         // SF7
#define LORA_CODINGRATE            1         // 1 = 4/5
#define LORA_PREAMBLE_LEN          8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
static RadioEvents_t RadioEvents;

bool loraIdle = true;
uint32_t seqPacote = 0;
float tensaoZeroACS_mV = 1250.0f;

// Acumuladores de energia (Wh)
float energiaComodo1_Wh   = 120.0f; // Inicia com valor base para visualizacao imediata no Dashboard
float energiaAreaComum_Wh = 80.0f;

unsigned long ultimaAmostra_ms = 0;
unsigned long ultimoEnvioLoRa_ms = 0;
bool enviarAreaComumAgora = false;

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void OnTxDone(void) {
  loraIdle = true;
}

void OnTxTimeout(void) {
  Radio.Sleep();
  loraIdle = true;
}

void lerADC(int pino, int &valorBruto, int &valorMilliVolts) {
  long somaBruto = 0;
  long somaMV = 0;
  const int amostras = 64;

  for (int i = 0; i < amostras; i++) {
    somaBruto += analogRead(pino);
    somaMV += analogReadMilliVolts(pino);
  }
  valorBruto = somaBruto / amostras;
  valorMilliVolts = somaMV / amostras;
}

void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  analogReadResolution(12);

  VextON();
  delay(100);
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.clear();
  display.drawString(0, 0, "NO MEDIDOR - INICIANDO");
  display.drawString(0, 16, "Calibrando ACS712 (0A)...");
  display.display();

  // Calibra tensao de repouso (0A) do ACS712
  int rawZero, mvZero;
  lerADC(PIN_ACS712, rawZero, mvZero);
  tensaoZeroACS_mV = (float)mvZero;

  // Configura Radio LoRa SX1262 em modo Transmissor
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LEN, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  Serial.println("\n=======================================================");
  Serial.println("  NO MEDIDOR LO-RA INICIADO (SENSORES + LORA 915 MHz)  ");
  Serial.println("=======================================================");

  ultimaAmostra_ms = millis();
  ultimoEnvioLoRa_ms = millis();
}

void loop() {
  Radio.IrqProcess();

  unsigned long agora_ms = millis();
  if (agora_ms - ultimaAmostra_ms < 1000) {
    return;
  }

  float deltaHoras = (agora_ms - ultimaAmostra_ms) / 3600000.0f;
  ultimaAmostra_ms = agora_ms;

  // 1. Leitura dos Sensores
  int rawACS, mvACS, rawPot, mvPot, rawLDR, mvLDR;
  lerADC(PIN_ACS712, rawACS, mvACS);
  lerADC(PIN_POT, rawPot, mvPot);
  lerADC(PIN_LDR, rawLDR, mvLDR);

  float diffACS_mV = abs((float)mvACS - tensaoZeroACS_mV);
  float correnteACS_A = diffACS_mV / SENSIBILIDADE_ACS_MV_POR_A;
  if (correnteACS_A < 0.08f) correnteACS_A = 0.0f;

  float correntePot_A = ((float)mvPot / 3300.0f) * CORRENTE_MAX_POT_A;
  bool luzAcesa = (mvLDR >= LIMIAR_LDR_MV);

  // 2. Calculo de Potencia (W) e Energia Acumulada (Wh)
  // Comodo 1: controlado pelo Potenciometro + ACS712 + LDR
  float correnteComodo1_A = correntePot_A + correnteACS_A;
  float potenciaComodo1_W = (TENSAO_REDE_V * correnteComodo1_A) + (luzAcesa ? POTENCIA_LAMPADA_W : 0.0f);
  energiaComodo1_Wh += potenciaComodo1_W * deltaHoras * 100.0f; // Fator acelerado para demonstracao ao vivo

  // Area Comum: carga base (geladeira/roteador) + iluminacao LDR + ACS712
  float potenciaAreaComum_W = POTENCIA_BASE_COMUM_W + (TENSAO_REDE_V * correnteACS_A) + (luzAcesa ? POTENCIA_LAMPADA_W : 0.0f);
  energiaAreaComum_Wh += potenciaAreaComum_W * deltaHoras * 100.0f;

  // 3. Disparo via LoRa a cada 2 segundos
  if (loraIdle && (agora_ms - ultimoEnvioLoRa_ms >= 2000)) {
    ultimoEnvioLoRa_ms = agora_ms;
    seqPacote++;

    const char* nodeId  = NODE_ID_PADRAO;
    const char* tipo    = TIPO_PADRAO;
    const char* morador = MORADOR_PADRAO;
    float potEnvio_W    = potenciaComodo1_W;
    float corrEnvio_A   = potEnvio_W / TENSAO_REDE_V;
    float kwhEnvio      = energiaComodo1_Wh / 1000.0f;

    if (ALTERNAR_COMODOS_DEMO && enviarAreaComumAgora) {
      nodeId      = "area_comum";
      tipo        = "comum";
      morador     = "Compartilhado";
      potEnvio_W  = potenciaAreaComum_W;
      corrEnvio_A = potEnvio_W / TENSAO_REDE_V;
      kwhEnvio    = energiaAreaComum_Wh / 1000.0f;
    }
    enviarAreaComumAgora = !enviarAreaComumAgora;

    // Formato com prefixo UNIFESP_MED| para filtrar interferencia de outros grupos no Gateway
    // Seguido de node_id e do JSON padrao para o MQTT / Node-RED
    char pacoteLoRa[240];
    snprintf(pacoteLoRa, sizeof(pacoteLoRa),
             "UNIFESP_MED|%s|{\"node_id\":\"%s\",\"tipo\":\"%s\",\"morador\":\"%s\","
             "\"seq\":%lu,\"tensao_v\":%.1f,\"corrente_a\":%.2f,\"potencia_w\":%.1f,"
             "\"energia_kwh\":%.3f,\"luz_acesa\":%s}",
             nodeId, nodeId, tipo, morador,
             (unsigned long)seqPacote, TENSAO_REDE_V, corrEnvio_A, potEnvio_W,
             kwhEnvio, luzAcesa ? "true" : "false");

    Serial.printf("[LoRa TX #%lu] %s\n", (unsigned long)seqPacote, pacoteLoRa);

    display.clear();
    display.drawString(0, 0, "MEDIDOR TX -> " + String(nodeId));
    display.drawString(0, 13, "Potencia: " + String(potEnvio_W, 1) + " W");
    display.drawString(0, 26, "Consumo : " + String(kwhEnvio, 3) + " kWh");
    display.drawString(0, 39, "Luz LDR : " + String(luzAcesa ? "ACESA" : "APAGADA"));
    display.drawString(0, 52, "Pacote LoRa #" + String(seqPacote) + " enviado");
    display.display();

    loraIdle = false;
    Radio.Send((uint8_t *)pacoteLoRa, strlen(pacoteLoRa));
  }
}