# Agitador Clínico Controlado — Reto de diseño 1
**Sistemas Embebidos · Universidad EIA**

Prototipo funcional de un sistema electrónico de agitación controlada para homogeneización de muestras de sangre en laboratorio clínico.

---
## Arquitectura del circuito
 
El diseño está dividido en tres zonas claramente separadas:
 
### Zona 1 — Control (3.3V)
ESP32 con entradas (potenciómetro, pulsadores) y salidas de indicación (LEDs, displays). Los displays de 7 segmentos se manejan en modo **multiplexado por software** con transistores 2N3904 en el cátodo común de cada dígito.
 
### Zona 2 — Aislamiento galvánico
Tres optoacopladores **4N25** separan completamente la masa de control (GND\_ctrl) de la masa de potencia (GND\_pwr). Uno por señal: PWM, DIR\_A, DIR\_B.
 
### Zona 3 — Potencia (12V)
**Puente H** con cuatro transistores TIP120 Darlington que permiten invertir el sentido de giro del motor. Cada transistor incluye un diodo flyback **1N4007** para protección frente a picos inductivos. Un fusible de **3A** protege la línea de 12V.
 
---