/*
 * BIBLIOTECAS NECESSARIAS NA ARDUINO IDE:
 * 1. Heltec ESP32 Dev-Boards
 * 2. PubSubClient (por Nick O'Leary)
 */

#include "LoRaWan_APP.h"
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"

// ============================================================================
// CONFIGURACOES DE WI-FI
// ============================================================================
const char* WIFI_SSID     = "eaevei";
const char* WIFI_PASSWORD = "eaesuave";

// ============================================================================
// CONFIGURACOES DO BROKER MQTT (Compativel com o fluxo do Node-RED)
// ============================================================================
const char* MQTT_BROKER     = "broker.hivemq.com"; // Ou IP do PC rodando Mosquitto
const int   MQTT_PORT       = 1883;
const char* MQTT_BASE_TOPIC = "unifesp/idc/medidor/";

// --- Parametros RF LoRa ---
#define RF_FREQUENCY               915000000 // 915 MHz (ANATEL Brasil)
#define LORA_BANDWIDTH             0         // 0 = 125 kHz
#define LORA_SPREADING_FACTOR      7         // SF7
#define LORA_CODINGRATE            1         // 1 = 4/5
#define LORA_PREAMBLE_LEN          8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
static RadioEvents_t RadioEvents;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool loraIdle = true;
bool novoPacoteRecebido = false;

char ultimoNodeId[32]   = "";
char ultimoJson[220]    = "";
int  ultimoRSSI         = 0;
int  ultimoSNR          = 0;
uint32_t totalRecebidos = 0;

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

// Callback executado quando chega um pacote LoRa na antena
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  // Buffer seguro de 256 bytes para evitar stack smashing com pacotes externos
  char msg[256];
  uint16_t tamCopiar = (size < sizeof(msg) - 1) ? size : (sizeof(msg) - 1);
  memcpy(msg, payload, tamCopiar);
  msg[tamCopiar] = '\0';
  Radio.Sleep();

  // Filtra apenas pacotes do projeto (prefixo "UNIFESP_MED|")
  if (strncmp(msg, "UNIFESP_MED|", 12) == 0) {
    char *ptrNode = msg + 12;
    char *sepJson = strchr(ptrNode, '|');

    if (sepJson != NULL) {
      *sepJson = '\0'; // Separa o node_id do JSON
      char *ptrJson = sepJson + 1;

      strncpy(ultimoNodeId, ptrNode, sizeof(ultimoNodeId) - 1);
      ultimoNodeId[sizeof(ultimoNodeId) - 1] = '\0';

      strncpy(ultimoJson, ptrJson, sizeof(ultimoJson) - 1);
      ultimoJson[sizeof(ultimoJson) - 1] = '\0';

      ultimoRSSI = rssi;
      ultimoSNR  = snr;
      totalRecebidos++;
      novoPacoteRecebido = true;
    }
  } else {
    Serial.printf("[LoRa RX Ignorado] Pacote externo de outro grupo (%u bytes): %s\n", size, msg);
  }

  loraIdle = true;
}

void conectarWiFi() {
  display.clear();
  display.drawString(0, 0, "GATEWAY LORA -> MQTT");
  display.drawString(0, 16, "Conectando ao Wi-Fi:");
  display.drawString(0, 30, WIFI_SSID);
  display.display();

  Serial.printf("\n[Wi-Fi] Conectando a %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[Wi-Fi] Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
    ArduinoOTA.setHostname("heltec-gateway-iot");
    ArduinoOTA.begin();
  } else {
    Serial.println("\n[Wi-Fi] Nao conectado (operando em modo somente LoRa RX local).");
  }
}

unsigned long ultimaTentativaMQTT_ms = 0;

void verificarConexaoMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqttClient.connected()) return;

  if (millis() - ultimaTentativaMQTT_ms < 10000) return;
  ultimaTentativaMQTT_ms = millis();

  espClient.setTimeout(2);
  Serial.printf("[MQTT] Conectando ao broker %s...\n", MQTT_BROKER);
  String clientId = "HeltecGateway-UNIFESP-" + String(random(0xffff), HEX);
  if (mqttClient.connect(clientId.c_str())) {
    Serial.println("[MQTT] Conectado ao broker com sucesso!");
  } else {
    Serial.printf("[MQTT] Aviso: Broker indisponivel (rc=%d).\n", mqttClient.state());
  }
}

void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  VextON();
  delay(100);
  display.init();
  display.setFont(ArialMT_Plain_10);

  conectarWiFi();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setBufferSize(512);

  // Configura Radio LoRa SX1262 em modo Receptor Continuo
  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LEN,
                    0, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  display.clear();
  display.drawString(0, 0, "GATEWAY LORA -> MQTT");
  display.drawString(0, 16, "Wi-Fi: " + String(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "Offline"));
  display.drawString(0, 32, "Aguardando pacotes LoRa");
  display.drawString(0, 46, "em 915 MHz...");
  display.display();

  Serial.println("\n=======================================================");
  Serial.println("  GATEWAY LORA + MQTT INICIADO (AGUARDANDO PACOTES)    ");
  Serial.println("=======================================================");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
    verificarConexaoMQTT();
    mqttClient.loop();
  }

  if (loraIdle) {
    loraIdle = false;
    Radio.Rx(0);
  }
  Radio.IrqProcess();

  // Quando um pacote valido do No Medidor chega via LoRa:
  if (novoPacoteRecebido) {
    novoPacoteRecebido = false;

    String topicoMQTT = String(MQTT_BASE_TOPIC) + String(ultimoNodeId);
    bool mqttOk = false;

    if (mqttClient.connected()) {
      mqttOk = mqttClient.publish(topicoMQTT.c_str(), ultimoJson);
    }

    Serial.printf("[Gateway RX #%lu] Origem: %s | RSSI: %d dBm | SNR: %d dB | MQTT (%s): %s\n",
                  (unsigned long)totalRecebidos, ultimoNodeId, ultimoRSSI, ultimoSNR,
                  topicoMQTT.c_str(), mqttOk ? "PUBLICADO" : "OFFLINE");
    Serial.printf("   JSON: %s\n", ultimoJson);

    display.clear();
    display.drawString(0, 0, "GW RX #" + String(totalRecebidos) + " [" + String(ultimoNodeId) + "]");
    display.drawString(0, 13, "RSSI:" + String(ultimoRSSI) + "dBm | SNR:" + String(ultimoSNR) + "dB");
    display.drawString(0, 26, "MQTT: " + String(mqttOk ? "Publicado OK" : "Sem Wi-Fi"));
    display.drawString(0, 39, "Topico: ../" + String(ultimoNodeId));
    display.drawString(0, 52, String(ultimoJson).substring(0, 24) + "...");
    display.display();
  }
}

