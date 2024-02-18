#include <M5Atom.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Kraken.h"
#include "M5_ENV.h"

// LED colors
#define LED_GREEN 0x00f000
#define LED_RED 0xf00000
#define LED_BLUE 0x0000f0
#define LED_PURPLE 0xf000f0
#define LED_WHITE 0xf0f0f0

// Device Type
#define DEVICE_TYPE "atomenv3"
#define RESET_INTERVAL_MSEC 60 * 60 * 1000   // Device will reset each 60min.

// WiFi
#define WIFI_SSID "[YOUR_SSID]"
#define WIFI_PASS "[YOUR_PASSWORD]"
// MQTT
#define MQTT_PORT 1883
#define MQTT_TOPIC "kraken"
#define MQTT_HOST "[HOST]"
// Other Settings
#define DEBUG_MODE 0                // 0: Release 1: Debug (WiFi/MQTT do not establish)

// Setup clients
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
// MQTT Client ID
String clientId = "KRKNATOMENV3_" + String(random(0xffff), HEX) + String(random(0xffff), HEX);
// Kraken
bool DEVICE_STARTED = false;
Kraken kraken;
// ENV3
SHT3X sht30;
QMP6988 qmp6988;
float tmp      = 0.0;
float hum      = 0.0;
float pressure = 0.0;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Message arrived [%s]\n", topic);
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
}

void mqttLoop() {
  if (!mqttClient.connected()) {
    M5.dis.drawpix(0, LED_RED);
    Serial.printf("MQTT disconnected. Retry to connect...\n");
    connectMqtt();
  }
  mqttClient.loop();
}

void connectMqtt() {
  int retryCount = 0;
  Serial.printf("%s\n", clientId.c_str());
  while (!mqttClient.connected()) {
    if (mqttClient.connect(clientId.c_str())) {
      M5.dis.drawpix(0, LED_WHITE);
      Serial.printf("Connected to Server\n", WiFi.localIP());
    } else {
      M5.dis.drawpix(0, LED_RED);
      Serial.printf("Connection error %d\n", mqttClient.state());
      // Wait 5 seconds before retrying
      delay(5000);
      if (retryCount > 4) esp_restart(); // reboot device.
      ++retryCount;
    }
  }
}

void connectWifi() {
  int retryCount = 0;
  Serial.printf("Connecting to %s ", String(WIFI_SSID).c_str());
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    M5.dis.drawpix(0, LED_RED);
    delay(500);
    Serial.print(".");
    ++retryCount;
    if (retryCount > 30) {
      Serial.print("Can not connect to WiFi. Reboot device.\n");
      esp_restart(); // reboot device.
    }
  }
  M5.dis.drawpix(0, LED_WHITE);
  Serial.printf("Connected!\n");
}

void publish(float tmp, float hum, float pressure, char *topic) {
  DynamicJsonDocument doc(1024);
  doc["type"] = "env3";
  doc["tmp"] = tmp;
  doc["hum"] = hum;
  doc["pressure"] = pressure;
  doc["device"] = DEVICE_TYPE;
  String json;
  serializeJson(doc, json);
  Serial.printf("%s\n", json.c_str());
  // Publish a message to MQTT server
  if (DEBUG_MODE == 0) {
    if (mqttClient.publish(topic, json.c_str())) {
      M5.dis.drawpix(0, LED_GREEN);
      Serial.printf("MQTT Message published!\n");
    } else {
      M5.dis.drawpix(0, LED_PURPLE);
      Serial.printf("ERROR: MQTT Message was not published...\n");
    }
    delay(1000);
    M5.dis.drawpix(0, LED_WHITE);
  }
  doc.clear();
  json.clear();
}

void setup() {
    Serial.begin(115200);
    M5.begin(true, false, true);
    Wire.begin(26, 32);
    qmp6988.init();
    Serial.println(F("ENVIII Unit(SHT30 and QMP6988)"));
    sht30.init();
    if (DEBUG_MODE == 0) {
        // WiFi
        connectWifi();
        Serial.printf("IP %s\n", WiFi.localIP().toString().c_str());
        // MQTT
        mqttClient.setServer(MQTT_HOST, MQTT_PORT);
        mqttClient.setCallback(mqttCallback);
        connectMqtt();
        kraken.Init(&Serial, &mqttClient, MQTT_TOPIC, DEVICE_TYPE, DEBUG_MODE);
    } else {
        Serial.printf("DEBUG: NO MQTT\n");
    }
}

void loop() {
    if (DEBUG_MODE == 0) {
        mqttLoop();
        if (!DEVICE_STARTED) {
            kraken.sendStatus(String("device_started"), 1, MQTT_TOPIC);
            DEVICE_STARTED = true;
        }
        if (millis() > RESET_INTERVAL_MSEC) {
            kraken.sendStatus(String("device_reset"), 2, MQTT_TOPIC);
            delay(100);
            esp_restart();
        }
    }
    pressure = qmp6988.calcPressure();
    if (sht30.get() == 0) {
        tmp = sht30.cTemp;
        hum = sht30.humidity;
    } else {
        tmp = 0, hum = 0;
    }
    publish(tmp, hum, pressure, MQTT_TOPIC);
    Serial.printf(
        "Temp: %2.1f  \r\nHumi: %2.0f%%  \r\nPressure:%2.0fPa\r\n---\n", tmp,
        hum, pressure);
    M5.dis.drawpix(0, LED_WHITE);
    delay(10000);
}