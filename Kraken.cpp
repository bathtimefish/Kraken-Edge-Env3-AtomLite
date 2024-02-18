#include <ArduinoJson.h>
#include "Kraken.h"

void Kraken::Init(HardwareSerial *serial, PubSubClient *mqtt, char *topic, String DEVTYPE, int DEBUG) {
  _serial = serial;
  _mqtt = mqtt;
  _topic = topic;
  _DEVTYPE = DEVTYPE;
  _DEBUG = DEBUG;
}

void Kraken::sendStatus(String status, int status_code, char *topic) {
  DynamicJsonDocument doc(1024);
  doc["type"] = "kraken";
  doc["status"] = status;
  doc["scode"] = String(status_code);
  doc["device"] = String(_DEVTYPE);
  String json;
  serializeJson(doc, json);
  // Publish a message to MQTT server
  if (_DEBUG == 0) {
    if (_mqtt->publish(_topic, json.c_str())) {
      _serial->printf("MQTT Message published!\n");
    } else {
      _serial->printf("ERROR: MQTT Message was not published...\n");
    }
  }
  doc.clear();
  json.clear();
}