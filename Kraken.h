#include <Arduino.h>
#include <PubSubClient.h>

class Kraken {
  private:
    HardwareSerial *_serial;
    PubSubClient *_mqtt;
    char *_topic;
    String _DEVTYPE;
    int _DEBUG;
  public:
    void Init(HardwareSerial *serial, PubSubClient *mqtt, char *topic, String DEVTYPE, int DEBUG);
    void sendStatus(String status, int status_code, char *topic);
};