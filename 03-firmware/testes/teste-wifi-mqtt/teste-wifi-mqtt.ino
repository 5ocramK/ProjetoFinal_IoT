/*
 * BIBLIOTECAS NECESSARIAS NA ARDUINO IDE:
 * 1. Heltec ESP32 Dev-Boards (para o display OLED HT_SSD1306Wire.h)
 * 2. PubSubClient (por Nick O'Leary - instalar no Gerenciador de Bibliotecas)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"

// ============================================================================
// CONFIGURACOES DE REDE WI-FI
// ============================================================================
const char* WIFI_SSID     = "eaevei";
const char* WIFI_PASSWORD = "eaesuave";

// ============================================================================
// CONFIGURACOES DO BROKER MQTT
// (Broker publico gratuito para teste imediato com o Node-RED)
// ============================================================================
const char* MQTT_BROKER   = "broker.hivemq.com"; // Ou "test.mosquitto.org"
const int   MQTT_PORT     = 1883;
const char* TOPIC_TESTE   = "unifesp/idc/medidor/comodo_1";

// Versao do Firmware
#define FIRMWARE_VERSION "v1.0"

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long ultimoEnvio_ms = 0;
int contadorMsg = 0;

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void conectarWiFi() {
  display.clear();
  display.drawString(0, 0, "TESTE WIFI + MQTT + OTA");
  display.drawString(0, 16, "Conectando ao Wi-Fi:");
  display.drawString(0, 30, WIFI_SSID);
  display.display();

  Serial.printf("\n[Wi-Fi] Conectando a %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.printf("\n[Wi-Fi] Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
}

void configurarOTA() {
  ArduinoOTA.setHostname("heltec-medidor-iot");

  ArduinoOTA.onStart([]() {
    display.clear();
    display.drawString(0, 0, "ATUALIZACAO OTA");
    display.drawString(0, 20, "Recebendo Firmware...");
    display.display();
    Serial.println("\n[OTA] Iniciando atualizacao remota de firmware...");
  });

  ArduinoOTA.onEnd([]() {
    display.clear();
    display.drawString(0, 0, "ATUALIZACAO OTA");
    display.drawString(0, 24, "Concluido! Reiniciando");
    display.display();
    Serial.println("\n[OTA] Atualizacao concluida com sucesso!");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Erro[%u]\n", error);
  });

  ArduinoOTA.begin();
  Serial.println("[OTA] Pronto para receber upload sem fio pela rede Wi-Fi!");
}

void reconectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.printf("[MQTT] Conectando ao broker %s:%d...\n", MQTT_BROKER, MQTT_PORT);
    String clientId = "HeltecV3-UNIFESP-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("[MQTT] Conectado ao broker com sucesso!");
    } else {
      Serial.printf("[MQTT] Falha (rc=%d). Tentando novamente em 2s...\n", mqttClient.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  VextON();
  delay(100);

  display.init();
  display.setFont(ArialMT_Plain_10);

  conectarWiFi();
  configurarOTA();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setBufferSize(512);
}

void loop() {
  // Mantem o servico de atualizacao remota OTA ativo
  ArduinoOTA.handle();

  // Garante conexao ativa com o broker MQTT
  if (!mqttClient.connected()) {
    reconectarMQTT();
  }
  mqttClient.loop();

  // Publica um pacote JSON de teste a cada 3 segundos
  if (millis() - ultimoEnvio_ms >= 3000) {
    ultimoEnvio_ms = millis();
    contadorMsg++;

    float potenciaSimulada = 450.0f + (contadorMsg % 10) * 25.0f;
    float energiaSimulada  = contadorMsg * 0.015f;

    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"node_id\":\"comodo_1\",\"tipo\":\"individual\",\"morador\":\"Morador 1\","
             "\"seq\":%d,\"tensao_v\":127.0,\"corrente_a\":%.2f,\"potencia_w\":%.1f,"
             "\"energia_kwh\":%.3f,\"luz_acesa\":true}",
             contadorMsg, potenciaSimulada / 127.0f, potenciaSimulada, energiaSimulada);

    bool ok = mqttClient.publish(TOPIC_TESTE, payload);

    Serial.printf("[MQTT TX #%d] Status: %s | Topico: %s\nPayload: %s\n\n",
                  contadorMsg, ok ? "OK" : "ERRO", TOPIC_TESTE, payload);

    display.clear();
    display.drawString(0, 0, "MQTT + OTA (" + String(FIRMWARE_VERSION) + ")");
    display.drawString(0, 13, "IP: " + WiFi.localIP().toString());
    display.drawString(0, 26, "Broker: HiveMQ (" + String(ok ? "OK" : "ERR") + ")");
    display.drawString(0, 39, "Envios MQTT: #" + String(contadorMsg));
    display.drawString(0, 52, "Pot: " + String(potenciaSimulada, 0) + "W | " + String(energiaSimulada, 3) + "kWh");
    display.display();
  }
}