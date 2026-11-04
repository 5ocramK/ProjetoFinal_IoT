# 02 — Circuito de Bancada e Placa de Circuito Impresso (PCI)

**Pontuação Relacionada:**
* **1,0 ponto:** Validação de circuito.
* **3,0 pontos (Obrigatório):** Projeto, confecção e documentação da Placa de Circuito Impresso (PCI).

---

## 1. Pinagem Padrão (Heltec WiFi LoRa 32 V3 — Nó Medidor)

Para que o esquema elétrico, a PCI e o firmware sigam o mesmo padrão entre todos os integrantes do grupo:

| Componente / Função | Pino no Heltec WiFi LoRa 32 V3 | Observação Técnica |
| :--- | :---: | :--- |
| **Alimentação Positiva (+3,3 V)** | Pino `3V3` | Nunca exceder 3,3 V nas portas analógicas (ADC) |
| **Referência (Terra)** | Pino `GND` | Comum a todos os sensores e capacitores |
| **Potenciômetro Cômodo 1 (Carga / Shunt)** | `GPIO 1` (`ADC1_CH0`) | Com resistor limitador (220 Ω) no 3,3V e capacitor de 100 nF para o GND |
| **LDR Cômodo 1 (Status de Iluminação)** | `GPIO 2` (`ADC1_CH1`) | Divisor de tensão: `3V3 -> LDR -> GPIO 2 -> Resistor 10 kΩ -> GND` |
| **Potenciômetro Cômodo 2 / Área Comum** *(Opcional)* | `GPIO 3` (`ADC1_CH2`) | Permite medir mais de um ambiente no mesmo nó |
| **LDR Cômodo 2 / Área Comum** *(Opcional)* | `GPIO 4` (`ADC1_CH3`) | Divisor de tensão: `3V3 -> LDR -> GPIO 4 -> Resistor 10 kΩ -> GND` |
| **LED Indicador de Consumo Ativo** *(Opcional)* | `GPIO 5` | Em série com resistor de 330 Ω para o GND |
| **Display OLED 0.96" (Interno da placa)** | `SDA: GPIO 17` / `SCL: GPIO 18` / `RST: GPIO 21` | Já roteado internamente na placa Heltec V3 (`Vext: GPIO 36`) |
| **Rádio LoRa SX1262 (Interno da placa)** | `NSS: 8`, `SCK: 9`, `MOSI: 10`, `MISO: 11`, `RST: 12`, `BUSY: 13`, `DIO1: 14` | Já roteado internamente na placa Heltec V3 |

---

## 2. Lista de Materiais (BOM) da PCI do Nó Medidor

* **1x** Placa **Heltec WiFi LoRa 32 V3** (com antena 915 MHz)
* **2x** Barras de pinos fêmea (*Female Headers*) de 18 pinos (passo 2,54 mm) para encaixar o Heltec V3 na PCI
* **1x a 2x** Potenciômetros lineares de 10 kΩ
* **1x a 2x** Fotoresistores (LDR 5 mm)
* **1x a 2x** Resistores de 10 kΩ (1/4 W) — para o divisor do LDR
* **1x a 2x** Resistores de 220 Ω ou 330 Ω (1/4 W) — proteção de fim de curso do potenciômetro / LED
* **2x** Capacitores cerâmicos de 100 nF (ou eletrolíticos de 10 µF) — filtragem de ruído nos pinos ADC

---

## 3. O que salvar nesta pasta

* `esquematico/` → Arquivo do diagrama esquemático (`.kicad_sch` / `.json` do EasyEDA) e exportação em **PDF / PNG** para o relatório e apresentação.
* `gerber/` → Arquivos `.zip` Gerber e visualização 2D/3D das trilhas da PCI.
* `fotos-validacao/` → Fotos do circuito funcionando na protoboard (validação de 1 pt) e fotos da **PCI confeccionada e soldada** (validação de 3 pts).
