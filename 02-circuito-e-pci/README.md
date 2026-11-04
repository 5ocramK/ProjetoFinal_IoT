# 02 — Circuito de Bancada e Placa de Circuito Impresso (PCI)

**Pontuação Relacionada:**
* **1,0 ponto:** Validação de circuito.
* **3,0 pontos (Obrigatório):** Projeto, confecção e documentação da Placa de Circuito Impresso (PCI).

---

## 1. Como ligar o ACS712 no Heltec WiFi LoRa 32 V3 / ESP32 (Divisor 5V → 3,3V)

O módulo **ACS712** é alimentado com **5 V** (`VCC = 5V`, disponível no pino `5V` da placa quando alimentada via USB) e entrega em seu pino `OUT`:
* **$2,5\text{ V}$** quando a corrente é **$0\text{ A}$** (repouso).
* Varia entre **$0\text{ V}$ e $5,0\text{ V}$** conforme a corrente passa pelos bornes `IP+` e `IP-` ($185\text{ mV/A}$ na versão de 5A, $100\text{ mV/A}$ na de 20A ou $66\text{ mV/A}$ na de 30A).

Como as portas analógicas (ADC) do **Heltec V3 / ESP32** suportam no máximo **3,3 V**, utilizamos **2 resistores** (divisor de tensão) na saída `OUT` do ACS712 antes de entrar no pino ADC:

```text
[ACS712 Pino OUT (0 a 5V)] ───[ Resistor R1 = 10 kΩ ]───┬───> [Pino GPIO 1 (ADC) do Heltec V3 (0 a 3,3V)]
                                                        │
                                           [ Resistor R2 = 20 kΩ ]  (ou dois de 10 kΩ em série)
                                           [ Capacitor C1 = 100 nF ] (em paralelo com R2 para filtrar ruído)
                                                        │
                                                      [GND]
```
* *(Observação: Se você usar dois resistores iguais de `10 kΩ` em `R1` e `R2`, também funciona perfeitamente: o `0–5V` vira `0–2,5V` no ADC, com ponto zero em `1,25V`, totalmente seguro para o Heltec V3!).*

---

## 2. Pinagem Padrão (Heltec WiFi LoRa 32 V3 — Nó Medidor)

| Componente / Função | Pino no Heltec WiFi LoRa 32 V3 | Observação Técnica |
| :--- | :---: | :--- |
| **Alimentação do ACS712 (+5 V)** | Pino `5V` (`VBUS`) | Alimenta o `VCC` do módulo ACS712 |
| **Alimentação Lógica (+3,3 V)** | Pino `3V3` | Alimenta o divisor do LDR e potenciômetro auxiliar |
| **Referência (Terra)** | Pino `GND` | Comum a todos os sensores, divisores e capacitores |
| **Saída do ACS712 (após divisor resistivo)** | `GPIO 1` (`ADC1_CH0`) | Mede a corrente real do cômodo com proteção de 3,3V e filtro de 100 nF |
| **LDR Cômodo 1 (Status de Iluminação)** | `GPIO 2` (`ADC1_CH1`) | Divisor de tensão: `3V3 -> LDR -> GPIO 2 -> Resistor 10 kΩ -> GND` |
| **Potenciômetro Auxiliar (Controle/Simulação de Carga)** | `GPIO 3` (`ADC1_CH2`) | Permite regular a carga na bancada ou simular um 2º cômodo / Área Comum |
| **LDR Cômodo 2 / Área Comum** *(Opcional)* | `GPIO 4` (`ADC1_CH3`) | Divisor de tensão: `3V3 -> LDR -> GPIO 4 -> Resistor 10 kΩ -> GND` |
| **Display OLED 0.96" (Interno da placa)** | `SDA: GPIO 17` / `SCL: GPIO 18` / `RST: GPIO 21` | Já roteado internamente na placa Heltec V3 (`Vext: GPIO 36`) |
| **Rádio LoRa SX1262 (Interno da placa)** | `NSS: 8`, `SCK: 9`, `MOSI: 10`, `MISO: 11`, `RST: 12`, `BUSY: 13`, `DIO1: 14` | Já roteado internamente na placa Heltec V3 |

---

## 3. Lista de Materiais (BOM) da PCI do Nó Medidor

* **1x** Placa **Heltec WiFi LoRa 32 V3** (com antena 915 MHz)
* **1x** Módulo sensor de corrente por efeito Hall **ACS712** (5A, 20A ou 30A)
* **2x** Barras de pinos fêmea (*Female Headers*) de 18 pinos para encaixar o Heltec V3 na PCI + barra de 3 pinos para o ACS712
* **1x** Potenciômetro linear de 10 kΩ (para controle fino de carga / simulação em bancada)
* **1x a 2x** Fotoresistores (LDR 5 mm)
* **3x a 4x** Resistores de 10 kΩ (1/4 W) — para o divisor de tensão do ACS712 (`5V -> 3,3V/2,5V`) e divisor do LDR
* **2x** Capacitores cerâmicos de 100 nF (ou eletrolíticos de 10 µF) — filtragem de ruído nos pinos ADC

---

## 4. O que salvar nesta pasta

* `esquematico/` → Arquivo do diagrama esquemático (`.kicad_sch` / `.json` do EasyEDA) e exportação em **PDF / PNG** para o relatório e apresentação.
* `gerber/` → Arquivos `.zip` Gerber e visualização 2D/3D das trilhas da PCI.
* `fotos-validacao/` → Fotos do circuito funcionando na protoboard (validação de 1 pt) e fotos da **PCI confeccionada e soldada com o ACS712 e Heltec V3** (validação de 3 pts).
