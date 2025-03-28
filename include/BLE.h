#ifndef BLE_H
#define BLE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

class BLELibrary {
public:
    const char* deviceId;
    bool init(const char* deviceName, BLECharacteristicCallbacks* callbacks);
    bool sendMessage(const String& message);
};

extern BLELibrary BLE;

#endif