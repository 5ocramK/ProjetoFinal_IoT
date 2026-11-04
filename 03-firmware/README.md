# 03 — Firmware (Nó Medidor LoRa TX e Gateway LoRa RX / MQTT)

**Requisitos Relacionados:**
* **Obrigatório:** Uso de comunicação LoRa (915 MHz).
* **Obrigatório (2,0 pts):** Envio para nuvem com uso de MQTT.
* **Desafio Extra (+1,0 pt):** Atualização remota de firmware (*OTA - Over-The-Air*).

---

## 1. Organização das Subpastas

* `no-medidor-comodo/` → Código do **Heltec WiFi LoRa 32 V3 (Transmissor — Cômodos e Área Comum)**:
  * Lê os pinos ADC (`GPIO 1` potenciômetro e `GPIO 2` LDR).
  * Converte a leitura analógica para Corrente (A), Potência Ativa (W) e integra no tempo para obter o Consumo Acumulado (Wh / kWh).
  * Atualiza o display OLED 0.96" local a cada 1 segundo.
  * Empacota os dados em JSON compacto e transmite via **LoRa 915 MHz (SX1262)** a cada 3 segundos.
  * Suporta atualização via Wi-Fi com `ArduinoOTA`.
* `no-principal-gateway/` → Código do **Heltec WiFi LoRa 32 V3 (Receptor / Gateway)**:
  * Escuta continuamente os pacotes **LoRa 915 MHz** vindos dos cômodos.
  * Exibe no display OLED o último nó recebido, potência, kWh e nível de sinal (`RSSI` e `SNR`).
  * Conecta ao Wi-Fi e publica o pacote JSON recebido no broker **MQTT**.
  * Suporta atualização via Wi-Fi com `ArduinoOTA`.
* `testes/` → Códigos de **teste isolado de bancada** antes da integração final:
  * `teste-sensores-acs712-ldr/` (validação e calibração do ACS712, potenciômetro e LDR no ADC).
  * `teste-lora-ping-pong/` (teste de alcance e perda de pacotes LoRa 915 MHz entre dois Heltec V3).
  * `teste-wifi-mqtt/` (teste isolado de publicação MQTT e upload OTA).

---

## 2. Padronização do Pacote LoRa e Payload MQTT (Contrato de Dados)

Para que quem for programar o **Firmware** e quem for montar o **Node-RED** trabalhem em paralelo sem conflito, os pacotes LoRa e MQTT seguirão exatamente este formato **JSON**:

```json
{
  "node_id": "comodo_1",
  "tipo": "individual",
  "morador": "Morador 1",
  "seq": 42,
  "tensao_v": 127.0,
  "corrente_a": 4.52,
  "potencia_w": 574.04,
  "energia_kwh": 1.245,
  "luz_acesa": true
}
```

Para o medidor da **Área Comum**, o campo `"tipo"` será `"comum"`:

```json
{
  "node_id": "area_comum",
  "tipo": "comum",
  "morador": "Compartilhado",
  "seq": 43,
  "tensao_v": 127.0,
  "corrente_a": 2.10,
  "potencia_w": 266.70,
  "energia_kwh": 0.860,
  "luz_acesa": false
}
```

---

## 3. Parâmetros RF LoRa (SX1262 — Heltec V3)

| Parâmetro | Valor Definido |
| :--- | :--- |
| **Frequência** | `915 MHz` (Padrão ISM ANATEL Brasil) |
| **Spreading Factor (SF)** | `SF7` |
| **Bandwidth (BW)** | `125 kHz` |
| **Coding Rate (CR)** | `4/5` |
| **Potência de Transmissão (TX)** | `14 dBm` |
| **Preamble Length** | `8 símbolos` |
| **CRC** | `Habilitado (ON)` |

