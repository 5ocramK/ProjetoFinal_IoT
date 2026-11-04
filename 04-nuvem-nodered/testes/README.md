# Testes da Etapa de Nuvem, MQTT e Node-RED (`04-nuvem-nodered/testes/`)

Esta pasta é destinada aos arquivos, payloads e fluxos de teste para validar o **Node-RED**, o **MQTT** e o **Banco de Dados** mesmo quando o hardware físico não estiver conectado.

---

## O que armazenar aqui

* **`payloads-simulacao-mqtt/`**
  * Exemplos de mensagens JSON simulando o consumo do `comodo_1`, `comodo_2` e `area_comum` para injetar via **MQTT Explorer** ou nós `inject` no Node-RED.
  * Usado para validar o **Requisito 4** (conferência matemática do rateio de conta com valores conhecidos, ex.: Quarto 1 = 10 kWh, Quarto 2 = 20 kWh, Área Comum = 12 kWh).
* **`teste-banco-de-dados/`**
  * Queries de teste de inserção (`INSERT`) e consulta histórica (`SELECT`) para comprovar a retenção dos dados após reiniciar o Node-RED (**Requisito 6 — Desafio Extra**).

