#include "comm.h"
#include <esp_task_wdt.h>

// 初始化 BLE 和 MQTT
void COMMLibrary::init(const char* deviceName, void (*callback)(const String&)) {
    BLE.init(deviceName, callback);
    MQTT.init(BLE.deviceId, callback);
}

// 发送消息
void COMMLibrary::sendMessage(const String& message) {
    if (BLE.isConnected()) BLE.sendMessage(message);
    else if (MQTT.isConnected()) return MQTT.sendMessage(message);
}

COMMLibrary COMM;