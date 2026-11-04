# 04 — Etapa de Nuvem (MQTT, Node-RED e Banco de Dados)

**Pontuação Relacionada:**
* **2,0 pontos (Obrigatório):** Validação de etapa de nuvem (com uso de MQTT) e uso obrigatório do **Node-RED**.
* **+1,0 ponto (Desafio Extra):** Implementação de Banco de Dados para histórico.

---

## 1. Estrutura de Tópicos MQTT

O **Heltec WiFi LoRa 32 V3 (Gateway)** publicará as leituras nos seguintes tópicos do broker MQTT (ex.: `test.mosquitto.org` ou broker local `localhost:1883`):

| Tópico MQTT | Descrição | Exemplo de Uso no Node-RED |
| :--- | :--- | :--- |
| `unifesp/idc/medidor/comodo_1` | Leituras de consumo do Quarto do Morador 1 | Consumo individual Morador 1 |
| `unifesp/idc/medidor/comodo_2` | Leituras de consumo do Quarto do Morador 2 | Consumo individual Morador 2 |
| `unifesp/idc/medidor/area_comum` | Leituras de consumo da Área Comum (Sala/Cozinha) | Dividido igualmente entre os moradores |
| `unifesp/idc/medidor/status_gateway` | Status do Gateway, IP e `RSSI`/`SNR` do último pacote LoRa | Monitoramento da qualidade do enlace LoRa |

*(Dica: No Node-RED, basta assinar o tópico curinga `unifesp/idc/medidor/#` para receber todos os cômodos em um único nó `mqtt in`!).*

---

## 2. Lógica de Rateio da Conta no Node-RED

O nó `function` do **Node-RED** manterá o último valor de `energia_kwh` de cada cômodo e aplicará a fórmula de divisão justa:

1. **Parcela da Área Comum por Morador:**
   $$\text{kWh}_{\text{Rateio Comum}} = \frac{\text{kWh}_{\text{Área Comum}}}{N_{\text{Moradores}}}$$
2. **Consumo Total Atribuído ao Morador $i$:**
   $$\text{kWh}_{\text{Total Morador } i} = \text{kWh}_{\text{Individual } i} + \text{kWh}_{\text{Rateio Comum}}$$
3. **Fatura Mensal do Morador $i$ em Reais (R\$):**
   $$\text{Fatura}_{\text{Morador } i}\text{ (R\$)} = \text{kWh}_{\text{Total Morador } i} \times \text{Tarifa (R\$/kWh)}$$

---

## 3. Componentes do Dashboard no Node-RED (`node-red-dashboard`)

* **Aba 1 — Visão Geral da Residência:**
  * Consumo total acumulado (kWh) e Fatura Total da Casa (R$).
  * Consumo da Área Comum (kWh) e status de iluminação (`Luz Acesa / Apagada`).
  * Qualidade do sinal LoRa (`RSSI` em dBm e `SNR` em dB).
* **Aba 2 — Divisão por Morador (Fatura Justa):**
  * **Card Morador 1:** Potência atual (W), Consumo do Quarto (kWh), Fração da Área Comum (kWh) e **Total a Pagar (R$)**.
  * **Card Morador 2:** Potência atual (W), Consumo do Quarto (kWh), Fração da Área Comum (kWh) e **Total a Pagar (R$)**.
  * Gráfico de Pizza (*Pie Chart*) comparando a participação de cada morador na conta do mês.
  * Gráfico Histórico de Linha (*Line Chart*) alimentado pelo **Banco de Dados (SQLite / InfluxDB)**.

---

## 4. O que salvar nesta pasta

* `fluxos-nodered/flows.json` → Exportação do fluxo completo do Node-RED (*Menu -> Export -> Clipboard -> Download*).
* `banco-de-dados/` → Script de criação da tabela SQLite/InfluxDB e captura de tela das consultas históricas.
* `testes/` → Payloads JSON de simulação (`payloads-simulacao-mqtt/`) para testar o cálculo de rateio sem depender do hardware ligado e scripts de validação do banco de dados (`teste-banco-de-dados/`).

