#include "comm.h"
#include <esp_task_wdt.h>

using callback = void (*)(const String&, const String&);
const char* mqtt_server = "broker-cn.emqx.io";
const int mqtt_port = 1883;
callback CommCallback = nullptr;

void MyCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
        String message = String(rxValue.c_str());
        Serial.println("通过 BLE 接收到: " + message);
        if (CommCallback != nullptr) CommCallback(message, "ble");
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println("通过 MQTT 接收到: " + message);
    if (CommCallback != nullptr) CommCallback(message, "mqtt");
}

void COMMLibrary::init(const char* deviceName, void (*cb)(const String&, const String&)) {
    CommCallback = cb;
    BLE.init(deviceName, new MyCallbacks());
    MQTT.init(mqtt_server, mqtt_port, BLE.deviceId, mqttCallback);
}

// 发送消息并返回状态
void COMMLibrary::sendMessage(const String& message, const String& via) {
    if (via == "ble") BLE.sendMessage(message);
    else if (via == "mqtt") return MQTT.sendMessage(message);
}

COMMLibrary COMM;