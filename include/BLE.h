#ifndef BLE_H
#define BLE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

class MyCallbacks : public BLECharacteristicCallbacks
{
public:
    void onWrite(BLECharacteristic *pCharacteristic) override;
};

class BLELibrary
{
public:
    String deviceId;
    void init(const char *deviceName, void (*callback)(const String &));
    void sendMessage(const String &message);
    bool isConnected();
};

extern BLELibrary BLE;

#endif