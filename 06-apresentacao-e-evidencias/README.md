# 06 — Apresentação e Evidências de Funcionamento do Sistema

**Pontuação Relacionada:**
* **3,0 pontos (Individual):** Apresentação e arguição.
* **Item Obrigatório 6:** Apresentação com as evidências de funcionamento do sistema.
* **Regra Obrigatória:** *"Cada apresentação deve constar qual foi a contribuição de cada membro."*

---

## 1. Checklist de Evidências (Fotos, Prints e Vídeos) para a Apresentação

Para comprovar a execução de todas as **Rotinas de Teste** descritas na planilha de requisitos, armazene nesta pasta:

- [ ] **Evidência 1 — Validação do Circuito (Potenciômetro + LDR):**
  * Foto do multímetro medindo a tensão no pino ADC em 3 posições do potenciômetro comparada com a tela OLED do Heltec V3.
  * Foto/vídeo do teste de iluminação no LDR (claro/escuro) alterando o status na tela OLED.
- [ ] **Evidência 2 — Comunicação LoRa 915 MHz:**
  * Foto do Heltec V3 Transmissor e do Heltec V3 Gateway operando à distância, exibindo os pacotes recebidos, `RSSI` e `SNR` no OLED e no Monitor Serial.
- [ ] **Evidência 3 — Etapa de Nuvem (MQTT + Node-RED):**
  * Captura de tela (print) do fluxo completo montado no **Node-RED** recebendo os tópicos MQTT.
  * Captura de tela do **Node-RED Dashboard** exibindo o consumo individual de cada morador, o consumo da área comum e o cálculo final em Reais (R$).
- [ ] **Evidência 4 — Placa de Circuito Impresso (PCI):**
  * Imagem do esquemático e layout 2D/3D da PCI.
  * Fotos da PCI confeccionada (frente e verso) com os componentes soldados e o Heltec V3 encaixado em pleno funcionamento.
- [ ] **Evidência 5 — Desafios Extras (Opcional, até +3 pts):**
  * Print da tabela do **Banco de Dados** com o histórico de leituras salvo.
  * Print/vídeo da atualização remota de firmware (**OTA**) concluída pelo Wi-Fi.
  * Foto da PCI montada dentro do **encapsulamento impresso em 3D**.

---

## 2. Registro Detalhado de Contribuição por Membro

Preencha abaixo o detalhamento que irá no slide de **Contribuição Individual** da apresentação:

### Integrante 1: Marco
* **Atividades executadas:**
  * Definição da arquitetura do medidor de energia inteligente com rateio via LoRa.
  * Levantamento de requisitos, especificações técnicas, rotinas de teste e critérios de aceitação.
  * Estruturação do repositório Git e documentação técnica.
  * *(Adicionar demais etapas realizadas)*

### Integrante 2: [Nome]
* **Atividades executadas:**
  * *(Descrever atividades realizadas)*

### Integrante 3: [Nome]
* **Atividades executadas:**
  * *(Descrever atividades realizadas)*
