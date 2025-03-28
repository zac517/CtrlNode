#ifndef COMM_H
#define COMM_H

#include <Arduino.h>
#include "BLE.h"
#include "MQTT.h"

class MyCallbacks : public BLECharacteristicCallbacks {
public:
    void onWrite(BLECharacteristic *pCharacteristic) override;
};

class COMMLibrary {
private:
    COMMLibrary() {}; // 私有构造函数
    static COMMLibrary* instance;

public:
    static COMMLibrary* getInstance();
    bool init(const char* deviceName, void (*cb)(const String&, const String&));
    bool sendMessage(const String& message, const String& via);  // 返回发送状态
};

extern COMMLibrary* COMM;

#endif