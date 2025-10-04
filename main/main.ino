#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "RTClib.h"
#include <SD.h>

// ---------- Configuración Wi-Fi ----------
const char* ssid = "MEGACABLE-2.4G-ADAB";
const char* password = "s2kU6cjveS";

// ---------- Configuración MQTT ----------
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_topic = "20217977_Andres_Arce";

WiFiClient espClient;
PubSubClient client(espClient);

// ---------- Configuración hardware ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS1307 rtc;

const int buzzerPin = 8;
const int sensorPin = 33;

// Pines LED RGB
const int redPin = 5;
const int greenPin = 6;
const int bluePin = 7;

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

      // Pines
      pinMode(sensorPin, INPUT);
      pinMode(buzzerPin, OUTPUT);
      pinMode(redPin, OUTPUT);
      pinMode(greenPin, OUTPUT);
      pinMode(bluePin, OUTPUT);

      // SD
      if (!SD.begin(5)) {
        Serial.println("Error al inicializar la tarjeta SD");
      } else {
        Serial.println("Tarjeta SD inicializada correctamente");
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
      Serial.println("Conectado a Wi-Fi");
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
      lcd.print(now.day());
      lcd.print("/");
      if (now.month() < 10) lcd.print("0");
      lcd.print(now.month());
      lcd.print("/");
      lcd.print(now.year());

      lcd.setCursor(0,1);
      if (now.hour() < 10) lcd.print("0");
      lcd.print(now.hour());
      lcd.print(":");
      if (now.minute() < 10) lcd.print("0");
      lcd.print(now.minute());
      lcd.print(":");
      if (now.second() < 10) lcd.print("0");
      lcd.print(now.second());
    }

    void setLEDColor(int red, int green, int blue) {
      digitalWrite(redPin, red == 255 ? HIGH : LOW);
      digitalWrite(greenPin, green == 255 ? HIGH : LOW);
      digitalWrite(bluePin, blue == 255 ? HIGH : LOW);
    }

    void storeData() {
      DateTime now = rtc.now();
      String filename = String(now.year()) + "_" + String(now.month()) + "_" + String(now.day()) + ".json";

      dataFile = SD.open(filename.c_str(), FILE_WRITE);
      if (dataFile) {
        dataFile.print("{");
        dataFile.print("\"fecha\":\"" + String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "\",");
        dataFile.print("\"hora\":\"" + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + "\",");
        dataFile.print("\"sensor\":\"" + String(sensorState ? "Tocado" : "Libre") + "\",");
        dataFile.print("\"buzzer\":\"" + String(sensorState ? "Activado" : "Desactivado") + "\"");
        dataFile.println("}");
        dataFile.close();
        Serial.println("Registro guardado en SD");
      } else {
        Serial.println("Error al abrir el archivo para escribir");
      }
    }

    void publishMQTT() {
      if (!client.connected()) reconnectMQTT();
      client.loop();

      DateTime now = rtc.now();
      String payload = "{";
      payload += "\"fecha\":\"" + String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "\",";
      payload += "\"hora\":\"" + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + "\",";
      payload += "\"sensor\":\"" + String(sensorState ? "Tocado" : "Libre") + "\",";
      payload += "\"buzzer\":\"" + String(sensorState ? "Activado" : "Desactivado") + "\"";
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
