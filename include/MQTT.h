#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <esp_task_wdt.h>
#include "WiFiLib.h"

class MQTTLibrary {
public:
    void init(const String& clientId, void (*callback)(const String &));
    void sendMessage(const String& message);
    bool isConnected();
};

extern MQTTLibrary MQTT;

#endif