#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "RTClib.h"
#include <FS.h>
#include <SD.h>

const char* ssid = "OPPO A60";
const char* password = "arce1234";

const char* mqtt_server = "test.mosquitto.org";
const int   mqtt_port   = 1883;
const char* mqtt_topic  = "20217977_Andres_Arce";

WiFiClient espClient;
PubSubClient client(espClient);

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS1307 rtc;

const int buzzerPin = 14;
const int sensorPin = 33;

// OJO: en ESP32 los pines 34 y 35 son de solo entrada
const int redPin   = 32;
const int greenPin = 25;   // cambia 35 -> 25 (salida)
const int bluePin  = 26;   // cambia 34 -> 26 (salida)

bool sensorState = false;
bool lastSensorState = false; 
unsigned long previousMillisSensor = 0;
const long intervalSensor = 50;  
unsigned long lastDebounceTime = 0; 
const long debounceDelay = 200;     
bool showDate = true;

File dataFile;

class Datalogger {
  public:
    Datalogger() {}

    void setup() {
      Serial.begin(115200);
      Wire.begin(21, 22);

      // RTC
      if (!rtc.begin()) {
        Serial.println("No se encuentra RTC");
        while (1);
      }
      if (!rtc.isrunning()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      }

      lcd.init();
      lcd.backlight();
      Serial.println("RTC inicializado correctamente");

      pinMode(sensorPin, INPUT);
      pinMode(buzzerPin, OUTPUT);
      pinMode(redPin, OUTPUT);
      pinMode(greenPin, OUTPUT);
      pinMode(bluePin, OUTPUT);

      // --- SD (forzamos 10 MHz) ---
      if (!SD.begin(5, SPI, 10000000)) {
        Serial.println("Error al inicializar la tarjeta SD");
      } else {
        uint8_t cardType = SD.cardType();
        Serial.print("SD OK. Tipo: ");
        if (cardType == CARD_NONE) Serial.println("Ninguna");
        else if (cardType == CARD_MMC) Serial.println("MMC");
        else if (cardType == CARD_SD) Serial.println("SDSC");
        else if (cardType == CARD_SDHC) Serial.println("SDHC/SDXC");
        else Serial.println("Desconocida");

        // Comprobar escritura básica
        File test = SD.open("/_test.tst", FILE_WRITE);
        if (!test) {
          Serial.println("No se pudo crear archivo de prueba (¿bloqueo físico? ¿formato no FAT32?)");
        } else {
          test.println("ok");
          test.close();
          SD.remove("/_test.tst");
          Serial.println("Prueba de escritura OK");
        }
      }

      // Wi-Fi y MQTT
      connectWiFi();
      client.setServer(mqtt_server, mqtt_port);
    }

    void connectWiFi() {
      Serial.print("Conectando a Wi-Fi...");
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println(" Conectado a Wi-Fi");
    }

    void reconnectMQTT() {
      while (!client.connected()) {
        Serial.print("Conectando a MQTT...");
        if (client.connect("ESP32_Datalogger")) {
          Serial.println("Conectado a MQTT");
        } else {
          Serial.print("Error, rc=");
          Serial.print(client.state());
          Serial.println(" Intentando de nuevo en 2s");
          delay(2000);
        }
      }
    }

    void readSensor() {
      bool reading = digitalRead(sensorPin);
      unsigned long currentMillis = millis();

      if (reading != lastSensorState) lastDebounceTime = currentMillis;

      if ((currentMillis - lastDebounceTime) > debounceDelay) {
        if (reading != sensorState) {
          sensorState = reading;

          // LCD
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Sensor: ");
          lcd.print(sensorState ? "Tocado" : "Libre");

          // Buzzer
          digitalWrite(buzzerPin, sensorState ? HIGH : LOW);

          // LED RGB
          if (sensorState) setLEDColor(255, 0, 0); // Rojo
          else setLEDColor(0, 255, 0);             // Verde

          // SD y MQTT
          storeData();
          publishMQTT();
        }
      }
      lastSensorState = reading;
    }

    void showDateTime() {
      DateTime now = rtc.now();
      lcd.clear();
      lcd.setCursor(0,0);
      if (now.day() < 10) lcd.print("0");
      lcd.print(now.day());  lcd.print("/");
      if (now.month() < 10) lcd.print("0");
      lcd.print(now.month()); lcd.print("/");
      lcd.print(now.year());

      lcd.setCursor(0,1);
      if (now.hour() < 10) lcd.print("0");
      lcd.print(now.hour());  lcd.print(":");
      if (now.minute() < 10) lcd.print("0");
      lcd.print(now.minute()); lcd.print(":");
      if (now.second() < 10) lcd.print("0");
      lcd.print(now.second());
    }

    void setLEDColor(int red, int green, int blue) {
      digitalWrite(redPin,   red   ? HIGH : LOW);
      digitalWrite(greenPin, green ? HIGH : LOW);
      digitalWrite(bluePin,  blue  ? HIGH : LOW);
    }

    // ---- AQUI el fix: nombre 8.3 y FILE_APPEND ----
    void storeData() {
      DateTime now = rtc.now();

      // nombre 8.3 -> /YYYYMMDD.jsn  (extensión de 3 letras)
      char filename[20];
      snprintf(filename, sizeof(filename), "/%04d%02d%02d.jsn",
               now.year(), now.month(), now.day());

      // Abrimos en append (crea si no existe)
      File f = SD.open(filename, FILE_APPEND);
      if (f) {
        // cada línea un objeto JSON (NDJSON)
        f.print("{");
        f.print("\"fecha\":\"");  print2(f, now.day());  f.print("/"); print2(f, now.month()); f.print("/"); f.print(now.year()); f.print("\",");
        f.print("\"hora\":\"");   print2(f, now.hour()); f.print(":");  print2(f, now.minute()); f.print(":"); print2(f, now.second()); f.print("\",");
        f.print("\"sensor\":\""); f.print(sensorState ? "Tocado" : "Libre"); f.print("\",");
        f.print("\"buzzer\":\""); f.print(sensorState ? "Activado" : "Desactivado"); f.print("\"");
        f.println("}");
        f.close();
        Serial.printf("Registro guardado en SD: %s\n", filename);
      } else {
        Serial.printf("Error al abrir el archivo para escribir: %s\n", filename);
        // Diagnóstico extra:
        if (!SD.begin(5, SPI, 10000000)) {
          Serial.println("Además, SD dejó de responder al reinicializar.");
        } else {
          Serial.println("SD responde; el problema es nombre/ruta/permisos.");
        }
      }
    }

    // helper para 2 dígitos
    void print2(Print& out, int v) {
      if (v < 10) out.print('0');
      out.print(v);
    }

    void publishMQTT() {
      if (!client.connected()) reconnectMQTT();
      client.loop();

      DateTime now = rtc.now();
      String payload = "{";
      payload += "\"fecha\":\"" + String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "\",";
      payload += "\"hora\":\""  + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + "\",";
      payload += "\"sensor\":\""+ String(sensorState ? "Tocado" : "Libre") + "\",";
      payload += "\"buzzer\":\""+ String(sensorState ? "Activado" : "Desactivado") + "\"";
      payload += "}";

      client.publish(mqtt_topic, payload.c_str());
      Serial.println("Mensaje publicado en MQTT");
    }

    void handleTasks() {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillisSensor >= intervalSensor) {
        previousMillisSensor = currentMillis;
        if (showDate) showDateTime();
        else readSensor();
        showDate = !showDate;
      }
    }
};

Datalogger datalogger;

void setup() {
  datalogger.setup();
}

void loop() {
  datalogger.handleTasks();
}
