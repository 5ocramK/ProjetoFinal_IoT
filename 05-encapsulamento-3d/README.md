# 05 — Projeto e Impressão de Encapsulamento 3D (Desafio Extra)

**Pontuação Relacionada:**
* **+1,0 ponto (Desafio Extra):** Projeto e impressão de encapsulamento.
* **Requisito Dimensional:** Espaçamento mínimo de 1 cm entre as bordas da PCI e as paredes internas do encapsulamento.

---

## 1. Especificações Mecânicas do Gabinete

O encapsulamento deve alojar a **Placa de Circuito Impresso (PCI)** confeccionada na etapa `02-circuito-e-pci` com o módulo **Heltec WiFi LoRa 32 V3** encaixado:

* **Folga interna (Requisito Dimensional):** Mínimo de **10 mm (1 cm)** entre as extremidades da PCI e as paredes internas da caixa.
* **Aberturas necessárias na tampa / laterais:**
  1. **Janela retangular superior:** Para visualização do display OLED 0.96" do Heltec V3.
  2. **Furo circular superior (Ø 7,5 mm):** Para passagem do eixo do potenciômetro (ajuste de carga do cômodo).
  3. **Furo circular superior (Ø 5,5 mm):** Para exposição do sensor LDR à iluminação do cômodo.
  4. **Abertura lateral:** Para acesso à porta USB-C de alimentação do Heltec V3 e passagem do conector/antena LoRa 915 MHz.

---

## 2. O que salvar nesta pasta

* Arquivos de modelagem CAD (`.f3d`, `.step` ou `.scad`).
* Arquivos de malha prontos para fatiamento e impressão 3D (`caixa_base.stl` e `caixa_tampa.stl`).
* Fotos da peça impressa em 3D com a PCI montada em seu interior.
