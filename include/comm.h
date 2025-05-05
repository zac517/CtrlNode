#ifndef COMM_H
#define COMM_H

#include <Arduino.h>
#include "BLE.h"
#include "MQTT.h"

class COMMLibrary {
public:
    void init(const char* deviceName, void (*callback)(const String&));
    void sendMessage(const String& message);
};

extern COMMLibrary COMM;

#endif