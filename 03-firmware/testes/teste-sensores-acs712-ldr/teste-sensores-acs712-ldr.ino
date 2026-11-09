/*
 * ============================================================================
 * PROJETO FINAL IoT - UNIFESP ICT SJC
 * Teste e Validação de Circuito (ACS712 + Potenciômetro + LDR + OLED)
 * Placa: Heltec WiFi LoRa 32 V3 (ESP32-S3)
 * ============================================================================
 *
 * PINAGEM NA PROTOBOARD:
 * - GPIO 1 (ADC): Saída OUT do ACS712 (passando por divisor 2x 10kΩ + cap 100nF)
 * - GPIO 2 (ADC): Divisor de tensão do LDR (3.3V -> LDR -> GPIO 2 -> 10kΩ -> GND)
 * - GPIO 3 (ADC): Pino central do Potenciômetro (extremos em 3.3V e GND)
 * - 5V  (VBUS)  : VCC do módulo ACS712
 * - 3V3         : Alimentação do LDR e Potenciômetro
 * - GND         : Terra comum de todo o circuito
 */

#include <Arduino.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"

// --- Definição dos Pinos Analógicos (ADC1 do Heltec V3 / ESP32-S3) ---
#define PIN_ACS712  1   // GPIO 1: Sensor de corrente ACS712 (com divisor resistivo)
#define PIN_LDR     2   // GPIO 2: Sensor de luminosidade LDR (status da lâmpada)
#define PIN_POT     3   // GPIO 3: Potenciômetro (ajuste/simulação de carga em bancada)

// --- Parâmetros Elétricos de Simulação e Calibração ---
#define TENSAO_REDE_V         127.0f  // Tensão padrão da residência (127V AC)
#define CORRENTE_MAX_POT_A    20.0f   // Corrente máxima simulada pelo potenciômetro (20A)
#define POTENCIA_LAMPADA_W    60.0f   // Potência somada quando o LDR detecta luz acesa (60W)
#define LIMIAR_LDR_MV         1500    // Acima de 1500 mV (1.50V) considera Lâmpada ACESA

// Sensibilidade do ACS712 antes do divisor (5A = 185 mV/A, 20A = 100 mV/A, 30A = 66 mV/A)
// Com divisor de 2 resistores iguais de 10kΩ (fator 0.5), a sensibilidade no pino ADC cai pela metade:
#define SENSIBILIDADE_ACS_MV_POR_A  92.5f  // Para ACS712 de 5A com divisor 10kΩ+10kΩ (185 * 0.5)

// --- Inicialização do Display OLED 0.96" nativo do Heltec V3 ---
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// --- Variáveis Globais ---
float tensaoZeroACS_mV = 1250.0f;  // Tensão de repouso (0A) calibrada no boot
float energiaAcumulada_Wh = 0.0f;  // Energia acumulada em Watt-hora (Wh)
unsigned long ultimoTempo_ms = 0;

// Função para ler média de amostras no ADC (reduz ruído para comparar com o multímetro)
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

// Liga a alimentação (Vext) do Display OLED no Heltec V3
void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW); // Vext é ativo em nível BAIXO no Heltec V3
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Configura resolução do ADC para 12 bits (0 a 4095)
  analogReadResolution(12);

  // Inicializa o Display OLED
  VextON();
  delay(100);
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.clear();
  display.drawString(0, 0, "UNIFESP - Projeto IoT");
  display.drawString(0, 16, "Calibrando ACS712...");
  display.drawString(0, 32, "Mantenha sem carga (0A)");
  display.display();

  // Calibra o ponto zero (0 A) do ACS712 na inicialização
  int rawZero, mvZero;
  lerADC(PIN_ACS712, rawZero, mvZero);
  tensaoZeroACS_mV = (float)mvZero;

  Serial.println("\n=======================================================");
  Serial.println("  PASSO 1: VALIDACAO DE CIRCUITO (ACS712 + POT + LDR)  ");
  Serial.println("=======================================================");
  Serial.printf("Ponto Zero calibrado do ACS712 (GPIO 1): %.1f mV (%.3f V)\n\n",
                tensaoZeroACS_mV, tensaoZeroACS_mV / 1000.0f);

  ultimoTempo_ms = millis();
  delay(1000);
}

void loop() {
  unsigned long agora_ms = millis();
  if (agora_ms - ultimoTempo_ms < 1000) {
    return; // Executa a cada 1 segundo (taxa de amostragem de 1 Hz do Requisito 1)
  }
  float deltaHoras = (agora_ms - ultimoTempo_ms) / 3600000.0f;
  ultimoTempo_ms = agora_ms;

  // 1. Leitura do ACS712 (GPIO 1)
  int rawACS, mvACS;
  lerADC(PIN_ACS712, rawACS, mvACS);
  float diffACS_mV = abs((float)mvACS - tensaoZeroACS_mV);
  float correnteACS_A = diffACS_mV / SENSIBILIDADE_ACS_MV_POR_A;
  if (correnteACS_A < 0.08f) correnteACS_A = 0.0f; // Filtro de zona morta (ruído em 0A)

  // 2. Leitura do Potenciômetro de Carga (GPIO 3)
  int rawPot, mvPot;
  lerADC(PIN_POT, rawPot, mvPot);
  float correntePot_A = ((float)mvPot / 3300.0f) * CORRENTE_MAX_POT_A;

  // 3. Leitura do LDR - Status de Iluminação do Cômodo (GPIO 2)
  int rawLDR, mvLDR;
  lerADC(PIN_LDR, rawLDR, mvLDR);
  bool luzAcesa = (mvLDR >= LIMIAR_LDR_MV);

  // 4. Cálculo da Corrente Total, Potência Ativa (W) e Energia Acumulada (Wh)
  float correnteTotal_A = correnteACS_A + correntePot_A;
  float potenciaAtiva_W = (TENSAO_REDE_V * correnteTotal_A) + (luzAcesa ? POTENCIA_LAMPADA_W : 0.0f);
  energiaAcumulada_Wh += potenciaAtiva_W * deltaHoras;

  // 5. Impressão detalhada no Monitor Serial (para o Teste com Multímetro da Planilha!)
  Serial.println("-------------------------------------------------------");
  Serial.printf("[GPIO 1 - ACS712] ADC: %4d | Tensao: %.3f V | Corrente ACS: %.2f A\n",
                rawACS, mvACS / 1000.0f, correnteACS_A);
  Serial.printf("[GPIO 3 - POTENC] ADC: %4d | Tensao: %.3f V | Corrente Sim: %.2f A\n",
                rawPot, mvPot / 1000.0f, correntePot_A);
  Serial.printf("[GPIO 2 - LDR   ] ADC: %4d | Tensao: %.3f V | Luz Comodo  : %s\n",
                rawLDR, mvLDR / 1000.0f, luzAcesa ? "ACESA (+60W)" : "APAGADA (0W)");
  Serial.printf(">> RESULTADO COMODO -> Potencia: %.1f W | Acumulado: %.3f Wh\n",
                potenciaAtiva_W, energiaAcumulada_Wh);

  // 6. Atualização do Display OLED 0.96" do Heltec V3
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "TESTE SENSORES - PASSO 1");
  display.drawString(0, 13, "ACS:" + String(mvACS / 1000.0f, 2) + "V | Pot:" + String(mvPot / 1000.0f, 2) + "V");
  display.drawString(0, 26, "LDR:" + String(mvLDR / 1000.0f, 2) + "V -> Luz:" + String(luzAcesa ? "ON" : "OFF"));
  display.drawString(0, 39, "Potencia: " + String(potenciaAtiva_W, 1) + " W");
  display.drawString(0, 52, "Energia : " + String(energiaAcumulada_Wh, 3) + " Wh");
  display.display();
}

