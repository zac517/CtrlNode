#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <esp_task_wdt.h>
#include "WiFiLib.h"

class MQTTLibrary {
public:
    bool init(const char* server, int port, const String& clientId, MQTT_CALLBACK_SIGNATURE);
    bool sendMessage(const String& message);
};

extern MQTTLibrary MQTT;

#endif