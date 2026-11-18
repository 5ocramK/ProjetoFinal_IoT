/*
 * ============================================================================
 * PROJETO FINAL IoT - UNIFESP ICT SJC
 * PLACA 1: Firmware do Nó Medidor (Sensores + OLED + LoRa 915MHz TX + OTA)
 * Arquivo: 03-firmware/no-medidor-comodo/no-medidor-comodo.ino
 * Placa na IDE: Heltec WiFi LoRa 32(V3)
 * ============================================================================
 *
 * PINAGEM NA PROTOBOARD:
 * - GPIO 1 (ADC): Saída OUT do ACS712 (divisor 2x 10kΩ + cap 100nF)
 * - GPIO 2 (ADC): Divisor de tensão do LDR (3.3V -> LDR -> GPIO 2 -> 10kΩ -> GND)
 * - GPIO 4 (ADC): Pino central do Potenciômetro (extremos em 3.3V e GND)
 * - 5V  (VBUS)  : VCC do módulo ACS712
 * - 3V3         : Alimentação do LDR e Potenciômetro
 * - GND         : Terra comum de todo o circuito
 */

#include "LoRaWan_APP.h"
#include <Arduino.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <WiFi.h>
#include <ArduinoOTA.h>

// ============================================================================
// 1. SELEÇÃO DO CÔMODO (1 = Quarto 1 | 2 = Quarto 2 | 3 = Área Comum)
// ============================================================================
#define PERFIL_NO 1

#if PERFIL_NO == 1
  #define NODE_ID   "comodo_1"
  #define TIPO_NO   "individual"
  #define MORADOR   "Morador 1"
#elif PERFIL_NO == 2
  #define NODE_ID   "comodo_2"
  #define TIPO_NO   "individual"
  #define MORADOR   "Morador 2"
#else
  #define NODE_ID   "area_comum"
  #define TIPO_NO   "comum"
  #define MORADOR   "Compartilhado"
#endif

// ============================================================================
// 2. CONFIGURAÇÃO WI-FI PARA OTA (Deixe false se não for usar OTA agora)
// ============================================================================
#define HABILITAR_OTA false
const char* WIFI_SSID = "SUA_REDE_WIFI";
const char* WIFI_PASS = "SUA_SENHA_WIFI";

// ============================================================================
// 3. PINOS ANALÓGICOS E PARÂMETROS ELÉTRICOS
// ============================================================================
#define PIN_ACS712  1   // GPIO 1: Sensor de corrente ACS712
#define PIN_LDR     2   // GPIO 2: Sensor de luminosidade LDR
#define PIN_POT     4   // GPIO 4: Potenciômetro de simulação de carga

#define TENSAO_REDE_V              127.0f // Tensão padrão (127V AC)
#define CORRENTE_MAX_POT_A         20.0f  // Corrente máxima simulada no potenciômetro (20A)
#define POTENCIA_LAMPADA_W         60.0f  // Potência somada quando LDR detecta luz acesa (60W)
#define LIMIAR_LDR_MV              1500   // Acima de 1500 mV (1.50V) considera Lâmpada ACESA
#define SENSIBILIDADE_ACS_MV_POR_A 92.5f  // ACS712 5A com divisor 10kΩ+10kΩ (185 * 0.5)

// ============================================================================
// 4. PARÂMETROS RF LoRa PADRONIZADOS (915 MHz, SF7, BW 125 kHz)
// ============================================================================
#define RF_FREQUENCY               915000000 // 915 MHz (ANATEL Brasil)
#define TX_OUTPUT_POWER            14        // 14 dBm
#define LORA_BANDWIDTH             0         // 0 = 125 kHz
#define LORA_SPREADING_FACTOR      7         // SF7
#define LORA_CODINGRATE            1         // 1 = 4/5
#define LORA_PREAMBLE_LEN          8         // 8 símbolos
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false

#define INTERVALO_LEITURA_MS       1000      // Lê sensores e atualiza OLED a cada 1s
#define INTERVALO_LORA_TX_MS       3000      // Envia pacote JSON via LoRa a cada 3s

// ============================================================================
// OBJETOS E VARIÁVEIS GLOBAIS
// ============================================================================
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
static RadioEvents_t RadioEvents;

bool loraIdle = true;
bool otaConectado = false;

float tensaoZeroACS_mV = 1250.0f;
float correnteTotal_A = 0.0f;
float potenciaAtiva_W = 0.0f;
float energiaAcumulada_Wh = 0.0f;
bool luzAcesa = false;

unsigned long seqPacote = 0;
unsigned long ultimoTempoLeitura_ms = 0;
unsigned long ultimoTempoLoRa_ms = 0;
char txPacket[256];

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
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

void OnTxDone(void) {
  Radio.Sleep();
  loraIdle = true;
}

void OnTxTimeout(void) {
  Radio.Sleep();
  loraIdle = true;
}

void iniciarOTA() {
  if (!HABILITAR_OTA) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[OTA] Conectando ao Wi-Fi");

  unsigned long inicioWiFi = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - inicioWiFi < 4000)) {
    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.setHostname(NODE_ID);
    ArduinoOTA.begin();
    otaConectado = true;
    Serial.printf("\n[OTA] Ativo! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[OTA] Sem Wi-Fi. Operando normalmente via LoRa 915MHz.");
  }
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
  display.drawString(0, 0, "UNIFESP - No Medidor");
  display.drawString(0, 16, "Calibrando ACS712...");
  display.drawString(0, 32, "Mantenha sem carga (0A)");
  display.display();

  int rawZero, mvZero;
  lerADC(PIN_ACS712, rawZero, mvZero);
  tensaoZeroACS_mV = (float)mvZero;

  Serial.println("\n=======================================================");
  Serial.printf(" NO MEDIDOR INICIADO: %s (%s)\n", NODE_ID, MORADOR);
  Serial.println("=======================================================");
  Serial.printf("Ponto Zero calibrado do ACS712 (GPIO 1): %.1f mV (%.3f V)\n\n",
                tensaoZeroACS_mV, tensaoZeroACS_mV / 1000.0f);

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LEN, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  iniciarOTA();

  ultimoTempoLeitura_ms = millis();
  ultimoTempoLoRa_ms = millis();
}

void loop() {
  if (otaConectado) {
    ArduinoOTA.handle();
  }

  unsigned long agora_ms = millis();

  // --------------------------------------------------------------------------
  // TAREFA 1: Leitura dos Sensores e Display OLED (a cada 1 segundo)
  // --------------------------------------------------------------------------
  if (agora_ms - ultimoTempoLeitura_ms >= INTERVALO_LEITURA_MS) {
    float deltaHoras = (agora_ms - ultimoTempoLeitura_ms) / 3600000.0f;
    ultimoTempoLeitura_ms = agora_ms;

    // 1. Leitura do ACS712 (GPIO 1)
    int rawACS, mvACS;
    lerADC(PIN_ACS712, rawACS, mvACS);
    float diffACS_mV = abs((float)mvACS - tensaoZeroACS_mV);
    float correnteACS_A = diffACS_mV / SENSIBILIDADE_ACS_MV_POR_A;
    if (correnteACS_A < 0.08f) correnteACS_A = 0.0f;

    // 2. Leitura do Potenciômetro de Carga (GPIO 4)
    int rawPot, mvPot;
    lerADC(PIN_POT, rawPot, mvPot);
    float correntePot_A = ((float)mvPot / 3300.0f) * CORRENTE_MAX_POT_A;

    // 3. Leitura do LDR (GPIO 2)
    int rawLDR, mvLDR;
    lerADC(PIN_LDR, rawLDR, mvLDR);
    luzAcesa = (mvLDR >= LIMIAR_LDR_MV);

    // 4. Cálculos elétricos
    correnteTotal_A = correnteACS_A + correntePot_A;
    potenciaAtiva_W = (TENSAO_REDE_V * correnteTotal_A) + (luzAcesa ? POTENCIA_LAMPADA_W : 0.0f);
    energiaAcumulada_Wh += potenciaAtiva_W * deltaHoras;

    // 5. Log detalhado no Monitor Serial
    Serial.println("-------------------------------------------------------");
    Serial.printf("[GPIO 1 - ACS712] ADC: %4d | Tensao: %.3f V | Corrente ACS: %.2f A\n",
                  rawACS, mvACS / 1000.0f, correnteACS_A);
    Serial.printf("[GPIO 4 - POTENC] ADC: %4d | Tensao: %.3f V | Corrente Sim: %.2f A\n",
                  rawPot, mvPot / 1000.0f, correntePot_A);
    Serial.printf("[GPIO 2 - LDR   ] ADC: %4d | Tensao: %.3f V | Luz Comodo  : %s\n",
                  rawLDR, mvLDR / 1000.0f, luzAcesa ? "ACESA (+60W)" : "APAGADA (0W)");
    Serial.printf(">> RESULTADO COMODO -> Potencia: %.1f W | Acumulado: %.3f Wh\n",
                  potenciaAtiva_W, energiaAcumulada_Wh);

    // 6. Atualiza Display OLED
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, String(NODE_ID) + " | TX Seq:" + String(seqPacote));
    display.drawString(0, 13, "ACS:" + String(mvACS / 1000.0f, 2) + "V | Pot:" + String(mvPot / 1000.0f, 2) + "V");
    display.drawString(0, 26, "I_Tot:" + String(correnteTotal_A, 2) + "A | Luz:" + String(luzAcesa ? "ON" : "OFF"));
    display.drawString(0, 39, "Potencia: " + String(potenciaAtiva_W, 1) + " W");
    display.drawString(0, 52, "Energia : " + String(energiaAcumulada_Wh, 3) + " Wh");
    display.display();
  }

  // --------------------------------------------------------------------------
  // TAREFA 2: Disparo do Pacote JSON via LoRa (a cada 3 segundos)
  // --------------------------------------------------------------------------
  if (loraIdle && (agora_ms - ultimoTempoLoRa_ms >= INTERVALO_LORA_TX_MS)) {
    ultimoTempoLoRa_ms = agora_ms;
    seqPacote++;

    snprintf(txPacket, sizeof(txPacket),
             "{\"node_id\":\"%s\",\"tipo\":\"%s\",\"morador\":\"%s\",\"seq\":%lu,"
             "\"tensao_v\":%.1f,\"corrente_a\":%.2f,\"potencia_w\":%.2f,"
             "\"energia_kwh\":%.4f,\"luz_acesa\":%s}",
             NODE_ID, TIPO_NO, MORADOR, seqPacote,
             TENSAO_REDE_V, correnteTotal_A, potenciaAtiva_W,
             energiaAcumulada_Wh / 1000.0f, luzAcesa ? "true" : "false");

    Serial.printf("\n>>> [LoRa TX #%lu] %s\n\n", seqPacote, txPacket);

    loraIdle = false;
    Radio.Send((uint8_t *)txPacket, strlen(txPacket));
  }

  Radio.IrqProcess();
}