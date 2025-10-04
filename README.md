# Proyecto Datalogger IoT con ESP32

## Descripción del Proyecto

Este proyecto consiste en un **datalogger IoT** desarrollado con un **ESP32**, diseñado para registrar y monitorear en tiempo real los eventos de un sensor táctil (XS-123). El sistema integra:

- **LCD 16x2 I2C** para visualización en tiempo real del estado del sensor.
- **LED RGB** para indicar el estado del sensor visualmente.
- **Buzzer** para señal acústica al detectar un toque.
- **Almacenamiento en MicroSD** en formato JSON para registros diarios.
- **Publicación MQTT** en tiempo real hacia un broker externo (`test.mosquitto.org`).

El proyecto cumple con todos los objetivos académicos del trabajo del datalogger y puede simularse o probarse en entornos de laboratorio.

---

## Objetivos

- Registrar eventos de un sensor táctil en tiempo real.  
- Indicar el estado del sensor mediante **LED RGB** y **buzzer**.  
- Visualizar información en un **LCD** de manera clara y legible.  
- Almacenar los registros en **tarjeta MicroSD** con formato JSON diario.  
- Publicar eventos en un broker **MQTT** para monitoreo remoto.  
- Gestionar tareas sin bloqueos usando **`millis()`** para temporización.  

---

## Hardware Utilizado

| Componente | Descripción | Pin/Conexión |
|------------|------------|--------------|
| ESP32 | Microcontrolador principal | - |
| Sensor táctil XS-123 | Detector de eventos de toque | GPIO33 |
| LED RGB | Indica visualmente el estado del sensor | Rojo: 32, Verde: 34, Azul: 35 |
| Buzzer | Señal acústica al detectar toque | 14 |
| LCD I2C 16x2 | Muestra estado del sensor | SDA: 21, SCL: 22 |
| MicroSD | Almacenamiento de registros JSON | CS: 5 |

---

## Software y Librerías

- **IDE:** Arduino IDE  
- **Lenguaje:** C++ para ESP32  
- **Librerías utilizadas:**  
  - `WiFi.h`  
  - `PubSubClient.h`  
  - `LiquidCrystal_I2C.h`  
  - `SD.h`  
  - `Wire.h`  
  - `RTClib.h`  

---

## Funcionalidades

1. **Sensor táctil XS-123**  
   - Detecta eventos de toque y envía el estado al sistema.  

2. **Buzzer y LED RGB**  
   - **Buzzer:** Se activa cuando el sensor es tocado.  
   - **LED RGB:** Rojo = tocado, Verde = libre.  

3. **LCD**  
   - Alterna la visualización entre estado del sensor y hora/fecha actual (RTC).  

4. **MicroSD**  
   - Guarda eventos de manera secuencial en archivos `<YYYY_MM_DD>.json`.  
   - Formato de cada registro:
   ```json
   {
       "fecha":"DD/MM/YYYY",
       "hora":"HH:MM:SS",
       "sensor":"Tocado/Libre",
       "buzzer":"Activado/Desactivado"
   }
