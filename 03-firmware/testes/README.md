# Testes Isolados de Firmware e Hardware (`03-firmware/testes/`)

Esta pasta é destinada aos **códigos de teste unitário/isolado** das partes do hardware antes de integrá-las no firmware final do Nó Medidor e do Gateway. Isso facilita encontrar erros de montagem e executar as **Rotinas de Teste** da planilha `Eng_Requisitos.xlsx`.

---

## Subpastas de Teste

* **`teste-sensores-acs712-ldr/`**
  * Código para testar isoladamente a leitura analógica (ADC) do sensor de corrente **ACS712** (com o divisor de tensão `5V → 3,3V`), do **potenciômetro** e do **LDR**, imprimindo os valores brutos (`0–4095`), a tensão (`V`) e a corrente (`A`) no Monitor Serial e no display OLED.
  * Usado para validar o **Requisito 1** (comparação com o multímetro).
* **`teste-lora-ping-pong/`**
  * Código para testar isoladamente o envio de pacotes numerados (`1 a 100`) via **LoRa 915 MHz (SX1262)** de um Heltec V3 para o outro, medindo a taxa de entrega de pacotes (*PDR*), `RSSI` e `SNR` entre cômodos.
  * Usado para validar o **Requisito 2** (alcance e integridade LoRa).
* **`teste-wifi-mqtt/`**
  * Código para testar isoladamente a conexão do **Heltec V3** no Wi-Fi, o envio de um payload JSON fixo para o broker **MQTT** e a atualização **OTA** (*Over-The-Air*).
  * Usado para validar o **Requisito 3** e o **Requisito 7 (Desafio Extra OTA)**.

