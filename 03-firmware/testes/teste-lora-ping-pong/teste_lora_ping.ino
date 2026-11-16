/*
 * ============================================================================
 * PROJETO FINAL IoT - UNIFESP ICT SJC
 * Teste Isolado de LoRa 915 MHz (Envio 1 a 100, PDR, RSSI e SNR)
 * Placa: Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262)
 * ============================================================================
 * Alterne a flag MODO_TRANSMISSOR:
 * - true  -> Placa Transmissora (Simula o Nó Medidor)
 * - false -> Placa Receptora (Simula o Nó Gateway)
 */

#include "LoRaWan_APP.h"
#include <Arduino.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"

#define MODO_TRANSMISSOR true  // true = TX (Cômodo) | false = RX (Gateway)

// --- Parâmetros RF Padronizados no Repositório ---
#define RF_FREQUENCY          915000000 // 915 MHz (ANATEL Brasil)
#define TX_OUTPUT_POWER       14        // 14 dBm
#define LORA_BANDWIDTH        0         // 0 = 125 kHz
#define LORA_SPREADING_FACTOR 7         // SF7
#define LORA_CODINGRATE       1         // 1 = 4/5
#define LORA_PREAMBLE_LEN     8         // 8 símbolos
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON  false
#define TOTAL_PACOTES_TESTE   100

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
static RadioEvents_t RadioEvents;

bool loraIdle = true;
int seqAtual = 0;
int pacotesRecebidos = 0;
int ultimoSeqRecebido = 0;
int ultimoRSSI = 0;
int ultimoSNR = 0;
unsigned long ultimoEnvio_ms = 0;

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

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  char msg[64];
  memcpy(msg, payload, size);
  msg[size] = '\0';
  Radio.Sleep();

  pacotesRecebidos++;
  ultimoRSSI = rssi;
  ultimoSNR = snr;
  sscanf(msg, "PING:%d", &ultimoSeqRecebido);

  float pdr = (ultimoSeqRecebido > 0) ? ((float)pacotesRecebidos / ultimoSeqRecebido) * 100.0f : 0.0f;

  Serial.printf("[RX] Pacote: %s | Recebidos: %d/%d (PDR: %.1f%%) | RSSI: %d dBm | SNR: %d dB\n",
                msg, pacotesRecebidos, ultimoSeqRecebido, pdr, ultimoRSSI, ultimoSNR);

  display.clear();
  display.drawString(0, 0, "TESTE LORA RX (GATEWAY)");
  display.drawString(0, 14, "Pacote: " + String(ultimoSeqRecebido) + " / 100");
  display.drawString(0, 27, "Recebidos: " + String(pacotesRecebidos) + " (PDR:" + String(pdr, 0) + "%)");
  display.drawString(0, 40, "RSSI: " + String(ultimoRSSI) + " dBm");
  display.drawString(0, 52, "SNR : " + String(ultimoSNR) + " dB");
  display.display();

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
  display.drawString(0, 0, MODO_TRANSMISSOR ? "LORA TX - INICIANDO" : "LORA RX - AGUARDANDO");
  display.display();

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = OnRxDone;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  if (MODO_TRANSMISSOR) {
    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LEN, LORA_FIX_LENGTH_PAYLOAD_ON,
                      true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
  } else {
    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE, 0, LORA_PREAMBLE_LEN,
                      0, LORA_FIX_LENGTH_PAYLOAD_ON,
                      0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  }
}

void loop() {
  if (MODO_TRANSMISSOR) {
    if (loraIdle && seqAtual < TOTAL_PACOTES_TESTE && (millis() - ultimoEnvio_ms >= 1000)) {
      ultimoEnvio_ms = millis();
      seqAtual++;
      char pacote[32];
      snprintf(pacote, sizeof(pacote), "PING:%d", seqAtual);

      Serial.printf("[TX] Enviando pacote %d/%d -> %s\n", seqAtual, TOTAL_PACOTES_TESTE, pacote);

      display.clear();
      display.drawString(0, 0, "TESTE LORA TX (COMODO)");
      display.drawString(0, 18, "Enviando pacote:");
      display.drawString(0, 34, String(seqAtual) + " de " + String(TOTAL_PACOTES_TESTE));
      display.display();

      loraIdle = false;
      Radio.Send((uint8_t *)pacote, strlen(pacote));
    }
  } else {
    if (loraIdle) {
      loraIdle = false;
      Radio.Rx(0);
    }
  }
  Radio.IrqProcess();
}
