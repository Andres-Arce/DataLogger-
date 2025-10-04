# Proyecto Datalogger IoT con ESP32

## Descripción del Proyecto

Este proyecto consiste en un **datalogger IoT** basado en **ESP32**, diseñado para **monitorear, registrar y visualizar eventos de un sensor táctil XS-123** en tiempo real.  
El sistema integra:

- **LCD I2C 16x2** para mostrar el estado del sensor y la hora del evento.  
- **LED RGB** para indicar visualmente el estado del sensor.  
- **Buzzer** para señal acústica al detectar un toque.  
- **MicroSD** para almacenar registros diarios en formato JSON.  
- **MQTT** (opcional) para transmitir eventos en tiempo real a un broker externo (`test.mosquitto.org`).  

El proyecto está pensado para **uso académico y simulación en Wokwi**, y sirve como base para sistemas de adquisición de datos en entornos inteligentes.

---

## Objetivos del Proyecto

- Diseñar un sistema de adquisición de datos autónomo.  
- Registrar eventos de un sensor táctil en **MicroSD** y MQTT.  
- Visualizar de forma simultánea en **LCD**, **LED RGB** y **buzzer**.  
- Implementar gestión de tareas **sin bloqueos** usando `millis()`.  
- Mantener registros ordenados y en formato estandarizado **JSON** para análisis posterior.  
- Permitir monitoreo remoto mediante MQTT.  

---

## Hardware Utilizado

| Componente | Función | Conexión |
|------------|--------|----------|
| ESP32 | Microcontrolador | - |
| Sensor táctil XS-123 | Detecta eventos de toque | GPIO33 |
| LED RGB | Indica estado del sensor | Rojo: 32, Verde: 34, Azul: 35 |
| Buzzer | Señal acústica | 14 |
| LCD I2C 16x2 | Visualización del estado | SDA: 21, SCL: 22 |
| MicroSD | Almacenamiento de registros | CS: 5 |

---

## Software y Librerías

- **IDE:** Arduino IDE  
- **Lenguaje:** C++  
- **Librerías:**  
  - `WiFi.h`  
  - `PubSubClient.h` (MQTT opcional)  
  - `LiquidCrystal_I2C.h`  
  - `SD.h`  
  - `Wire.h`  
  - `RTClib.h`  

---

## Lógica de Funcionamiento

1. **Lectura del Sensor**
   - El ESP32 detecta el estado del sensor táctil XS-123.  
   - Se implementa **debounce** para evitar falsos positivos.  
   - Solo se registran cambios de estado (Tocado → Libre o Libre → Tocado).

2. **Control de Salidas**
   - **LED RGB:**  
     - Rojo = Sensor tocado  
     - Verde = Sensor libre  
   - **Buzzer:**  
     - Activo cuando el sensor está tocado.

3. **Visualización en LCD**
   - Primera línea: Estado del sensor (`Tocado`/`Libre`)  
   - Segunda línea: Hora del evento proveniente del RTC  

4. **Almacenamiento en MicroSD**
   - Archivo diario: `<YYYY_MM_DD>.json`  
   - Registro JSON ejemplo:
   ```json
   {
       "fecha": "29/09/2025",
       "hora": "14:35:05",
       "sensor": "Tocado",
       "buzzer": "Activado"
   }

## Diagrama de Flujo del Datalogger

[Sensor táctil XS-123] 
        ↓
   Lectura del estado
        ↓
+--------------------+
| Comparar con estado|
| anterior (debounce)|
+--------------------+
        ↓
 Si hay cambio → Actualiza:
        ↓
+--------------------+       +-----------------+
| LED RGB            |       | Buzzer          |
| (Rojo/Verde)       |       | (ON/OFF)        |
+--------------------+       +-----------------+
        ↓
+--------------------+
| LCD 16x2           |
| Estado y hora      |
+--------------------+
        ↓
+--------------------+
| MicroSD            |
| Guarda registro    |
+--------------------+
        ↓
+--------------------+
| MQTT (opcional)    |
| Publica evento     |
+--------------------+

## Publicación en MQTT

- **Broker:** `test.mosquitto.org`  
- **Tópico:** `20217977_Andres_Arce`  
- Cada cambio de estado del sensor se publica como un mensaje JSON.  
- Reconexión automática si se pierde la conexión.

---

## Gestión de Tareas

- Se utiliza `millis()` para alternar entre la lectura del sensor y la actualización de la LCD.  
- Esto evita el uso de `delay()` y permite la ejecución paralela de tareas sin bloquear el sistema.

---



