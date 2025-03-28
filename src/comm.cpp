#include "comm.h"
#include <esp_task_wdt.h>

using callback = void (*)(const String&, const String&);
const char* mqtt_server = "broker-cn.emqx.io";
const int mqtt_port = 1883;
callback CommCallback = nullptr;

// 初始化静态成员
COMMLibrary* COMMLibrary::instance = nullptr;
// 全局指针而非对象
COMMLibrary* COMM = nullptr;

COMMLibrary* COMMLibrary::getInstance() {
    if (instance == nullptr) {
        instance = new COMMLibrary();
    }
    return instance;
}

void MyCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
        String message = String(rxValue.c_str());
        Serial.println("通过BLE接收到: " + message);
        if (CommCallback != nullptr) CommCallback(message, "ble");
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println("通过MQTT接收到: " + message);
    if (CommCallback != nullptr) CommCallback(message, "mqtt");
}

bool COMMLibrary::init(const char* deviceName, void (*cb)(const String&, const String&)) {
    CommCallback = cb;
    Serial.println("开始初始化通信组件...");
    
    // 重置看门狗定时器
    esp_task_wdt_reset();
    
    // 初始化BLE
    Serial.println("初始化BLE...");
    if (!BLE.init(deviceName, new MyCallbacks())) {
        Serial.println("BLE初始化失败");
        return false;
    }
    
    // 重置看门狗定时器
    esp_task_wdt_reset();
    
    // 初始化MQTT
    Serial.println("初始化MQTT...");
    if (!MQTT.init(mqtt_server, mqtt_port, "11223344", mqttCallback)) {
        Serial.println("MQTT初始化失败，但继续执行");
        // MQTT失败不影响整体功能
    }
    
    Serial.println("通信组件初始化完成");
    return true;
}

// 发送消息并返回状态
bool COMMLibrary::sendMessage(const String& message, const String& via) {
    if (via == "ble") {
        //return BLE.sendMessage(message);
    }
    else if (via == "mqtt") {
        return MQTT.sendMessage(message);
    }
    else {
        Serial.println("无活跃连接，无法发送消息: " + message);
        return false;
    }
}