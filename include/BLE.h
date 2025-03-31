#ifndef BLE_H
#define BLE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

class BLELibrary {
public:
    String deviceId;
    void init(const char* deviceName, BLECharacteristicCallbacks* callbacks);
    void sendMessage(const String& message);
};

extern BLELibrary BLE;

#endif