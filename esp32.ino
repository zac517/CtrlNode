#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Preferences.h>
#include <BLE2902.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

// PWM相关定义
#define LED 32  // 定义第一个LED引脚
#define LED2 33 // 定义第二个LED引脚
#define PWM_CHANNEL_1 0
#define PWM_CHANNEL_2 1
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

Preferences prefs;  // 创建Preferences对象，用于本地存储

#define WIFI_CONNECT_TIMEOUT 10000  // WiFi连接超时时间（毫秒）
#define SERVICE_UUID "00001810-0000-1000-8000-00805f9b34fb"  // BLE服务UUID
#define WRITE_CHARACTERISTIC_UUID "00002a56-0000-1000-8000-00805f9b34fb"  // 用于接收数据的特征值UUID
#define NOTIFY_CHARACTERISTIC_UUID "00002a57-0000-1000-8000-00805f9b34fb"  // 用于发送数据的特征值UUID

const char* mqtt_server = "broker-cn.emqx.io";  // MQTT服务器地址
const int mqtt_port = 1883;  // MQTT服务器端口

// 本地存储的键名
const char* stored_power = "power";  // 电源状态
const char* stored_wifi = "wifi";  // WiFi状态
const char* stored_ssid = "ssid";  // WiFi名称
const char* stored_password = "password";  // WiFi密码
const char* stored_mode = "mode";  // 模式
const char* stored_brightness = "brightness";  // 亮度
const char* stored_color = "color";  // 色温

// 设备状态变量
String power_state = "off";  // 电源状态，默认为"off"
String wifi_state = "off";  // WiFi状态，默认为"off"
String ssid = "";  // 保存的WiFi名称
String password = "";  // 保存的WiFi密码
int mode = 0;  // 模式，默认0
int brightness = 0;  // 亮度，默认0
int color = 0;  // 色温，默认0

// 临时WiFi凭证（用于"try"命令）
String temp_ssid = "";  // 临时WiFi名称
String temp_password = "";  // 临时WiFi密码

// 连接状态变量
bool ble_connected = false;  // 蓝牙连接状态
bool mqtt_connected = false; // MQTT连接状态

// 新增全局变量
String bleMacAddress;      // 存储蓝牙MAC地址
String mqtt_topic;         // 动态生成的MQTT主题

// BLE和MQTT客户端
WiFiClient espClient;
PubSubClient client(espClient);
BLECharacteristic* pWriteCharacteristic;  // 写特征值指针
BLECharacteristic* pNotifyCharacteristic;  // 通知特征值指针

// 函数声明
void setupBLE();  // 初始化BLE
void setupWiFi();  // 初始化WiFi
void connectMQTT();  // 连接MQTT
void saveConfig();  // 保存配置
void loadConfig();  // 加载配置
void sendDeviceState(const String& via);  // 发送设备状态
void handleReceivedMessage(const String& message, const String& via);  // 处理接收到的消息
void tryConnectWiFi(const String& via);  // 尝试连接WiFi
void sendMessage(const String& message, const String& via);  // 发送消息
void setLightsBrightness(int brightness, int color);

// MQTT任务函数
void mqttTask(void *pvParameters) {
    while (true) {
        if (wifi_state == "on" && WiFi.status() == WL_CONNECTED) {
            if (!client.connected()) {
                connectMQTT();
            }
            client.loop();  // 处理MQTT消息
        }
        esp_task_wdt_reset();  // 定期喂狗，防止看门狗重启
        delay(10);  // 短暂延时，避免任务过于频繁占用CPU
    }
}

// BLE服务器回调类
class MyServerCallbacks : public BLEServerCallbacks {
public:
    void onConnect(BLEServer* pServer) override {
        Serial.println("客户端通过BLE连接");
        ble_connected = true;  // 设置蓝牙连接状态为true
    }

    void onDisconnect(BLEServer* pServer) override {
        Serial.println("客户端从BLE断开");
        ble_connected = false;  // 设置蓝牙连接状态为false
        BLEDevice::startAdvertising();  // 断开后重新启动广播
        Serial.println("广播已重启");
    }
};

// BLE写特征值回调类
class MyCallbacks : public BLECharacteristicCallbacks {
public:
    void onWrite(BLECharacteristic *pCharacteristic) override {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
            String message = String(rxValue.c_str());
            Serial.println("通过BLE接收到: " + message);
            handleReceivedMessage(message, "ble");  // 处理接收到的消息
        }
    }
};

// MQTT回调函数
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println("通过MQTT接收到: " + message);
    handleReceivedMessage(message, "mqtt");  // 处理接收到的消息
}

void setup() {
    Serial.begin(115200);

    // 初始化PWM
    ledcSetup(PWM_CHANNEL_1, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LED, PWM_CHANNEL_1);
    ledcSetup(PWM_CHANNEL_2, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LED2, PWM_CHANNEL_2);

    // 初始化任务看门狗，超时时间设为30秒
    esp_task_wdt_init(30, true);
    esp_task_wdt_add(NULL);

    // 加载配置
    loadConfig();

    // 输出加载的配置
    Serial.println("加载的配置:");
    Serial.println("电源状态: " + power_state);
    Serial.println("WiFi状态: " + wifi_state);
    Serial.println("WiFi名称: " + ssid);
    Serial.println("WiFi密码: " + password);
    Serial.println("模式: " + String(mode));
    Serial.println("亮度: " + String(brightness));
    Serial.println("色温: " + String(color));

    // 根据电源状态设置LED
    if (power_state == "on") {
        setLightsBrightness(brightness, color);
    } else {
        ledcWrite(PWM_CHANNEL_1, 0);
        ledcWrite(PWM_CHANNEL_2, 0);
    }

    // 初始化BLE并获取MAC地址
    setupBLE();

    // 构造MQTT主题
    mqtt_topic = "Lumina/devices/" + bleMacAddress;
    Serial.print("MQTT主题: ");
    Serial.println(mqtt_topic);

    // 如果WiFi状态为"on"，尝试连接WiFi
    if (wifi_state == "on") {
        setupWiFi();
    }

    // 创建MQTT任务，运行在核心0
    xTaskCreatePinnedToCore(
        mqttTask,    // 任务函数
        "MQTT Task", // 任务名称
        4096,        // 堆栈大小
        NULL,        // 任务参数
        1,           // 任务优先级
        NULL,        // 任务句柄
        0            // 运行在核心0，主任务默认运行在核心1
    );
}

void loop() {
    static unsigned long lastMillis = 0;
    if (millis() - lastMillis >= 10) {
        if (Serial.available()) {
            String input = Serial.readStringUntil('\n');
            if (input == "CLEAR") {
                prefs.begin("config", false);
                prefs.clear();
                prefs.end();
                Serial.println("配置已清除");
                ESP.restart();
            }
        }
        lastMillis = millis();
        esp_task_wdt_reset();  // 主线程喂狗
    }
}

// 初始化BLE
void setupBLE() {
    BLEDevice::init("Lumina 智能台灯");  // 设置BLE设备名称
    // 获取并存储蓝牙MAC地址
    bleMacAddress = BLEDevice::getAddress().toString().c_str();
    Serial.print("蓝牙MAC地址: ");
    Serial.println(bleMacAddress);

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());  // 设置服务器回调

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pWriteCharacteristic = pService->createCharacteristic(
        WRITE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE  // 只允许写入
    );
    pWriteCharacteristic->setCallbacks(new MyCallbacks());

    pNotifyCharacteristic = pService->createCharacteristic(
        NOTIFY_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY  // 只允许通知
    );
    pNotifyCharacteristic->addDescriptor(new BLE2902());  // 添加通知描述符

    pService->start();

    BLEAdvertisementData advertisementData;
    advertisementData.setManufacturerData("Lumina");  // 设置广播数据
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();
    Serial.println("BLE已初始化，等待客户端连接...");
}

// 初始化WiFi
void setupWiFi() {
    if (ssid.length() > 0 && password.length() > 0) {
        Serial.println("连接WiFi: " + ssid);
        WiFi.disconnect(true);
        delay(100);
        WiFi.begin(ssid.c_str(), password.c_str());

        unsigned long startMillis = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - startMillis > WIFI_CONNECT_TIMEOUT) {
                Serial.println("WiFi连接失败");
                return;
            }
            esp_task_wdt_reset();  // 喂狗
            delay(100);
            yield(); // 释放CPU控制权
        }
        Serial.println("WiFi已连接");
        wifi_state = "on";  // 修改为"on"
        saveConfig();
    } else {
        Serial.println("没有保存的WiFi凭证");
        wifi_state = "off";
        saveConfig();
    }
}

// 连接MQTT（在mqttTask中调用）
void connectMQTT() {
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqttCallback);

    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    Serial.println("当前 mqtt 客户 Id 为：" + clientId);

    while (!client.connected()) {
        Serial.print("尝试MQTT连接...");
        if (client.connect(clientId.c_str())) {
            Serial.println("\nMQTT已连接");
            String controlTopic = mqtt_topic + "/control";  // 订阅 /control 主题
            client.subscribe(controlTopic.c_str());
            mqtt_connected = true;  // 更新MQTT连接状态
        } else {
            Serial.print("失败，rc=");
            Serial.print(client.state());
            Serial.println(" 3秒后重试...");
            delay(3000); // 适当延时避免快速重试
        }
        esp_task_wdt_reset(); // 喂狗
    }
}

// 保存配置到Preferences
void saveConfig() {
    prefs.begin("config", false);
    prefs.putString(stored_power, power_state);
    prefs.putString(stored_wifi, wifi_state);
    prefs.putString(stored_ssid, ssid);
    prefs.putString(stored_password, password);
    prefs.putInt(stored_mode, mode);
    prefs.putInt(stored_brightness, brightness);
    prefs.putInt(stored_color, color);
    prefs.end();
    Serial.println("配置已保存");
}

// 从Preferences加载配置
void loadConfig() {
    prefs.begin("config", true);
    power_state = prefs.getString(stored_power, "off");  // 默认"off"
    wifi_state = prefs.getString(stored_wifi, "off");  // 默认"off"
    ssid = prefs.getString(stored_ssid, "");  // 默认空
    password = prefs.getString(stored_password, "");  // 默认空
    mode = prefs.getInt(stored_mode, 0);  // 默认0
    brightness = prefs.getInt(stored_brightness, 0);  // 默认0
    color = prefs.getInt(stored_color, 0);  // 默认0
    prefs.end();
}

// 发送设备状态
void sendDeviceState(const String& via) {
    sendMessage("{\"power\": \"" + power_state + "\"}", via);
    sendMessage("{\"mode\": " + String(mode) + "}", via);
    sendMessage("{\"bn\": " + String(brightness) + "}", via);
    sendMessage("{\"color\": " + String(color) + "}", via);
    sendMessage("{\"wifi\": \"" + wifi_state + "\"}", via);
    if (wifi_state) {
        if (WiFi.status() == WL_CONNECTED) {
          sendMessage("{\"wifi\": \"true\"}", via);
        } else {
            sendMessage("{\"wifi\": \"false\"}", via);
        }
    }
}

// 处理接收到的消息（BLE和MQTT统一处理）
void handleReceivedMessage(const String& message, const String& via) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Serial.print("JSON解析失败: ");
        Serial.println(error.c_str());
        return;
    }

    if (!doc["power"].isNull()) {
        power_state = String(doc["power"].as<const char*>());
        Serial.println("通过" + via + "设置电源状态为 " + power_state);
        if (power_state == "on") {
            setLightsBrightness(brightness, color);
        } else {
            ledcWrite(PWM_CHANNEL_1, 0);
            ledcWrite(PWM_CHANNEL_2, 0);
        }
        saveConfig();
    } else if (!doc["mode"].isNull()) {
        mode = doc["mode"].as<int>();
        Serial.println("通过" + via + "设置模式为 " + String(mode));
        saveConfig();
    } else if (!doc["bn"].isNull()) {
        brightness = doc["bn"].as<int>();
        Serial.println("通过" + via + "设置亮度为 " + String(brightness));
        setLightsBrightness(brightness, color);
        saveConfig();
    } else if (!doc["color"].isNull()) {
        color = doc["color"].as<int>();
        Serial.println("通过" + via + "设置色温为 " + String(color));
        setLightsBrightness(brightness, color);
        saveConfig();
    } else if (!doc["wifi"].isNull()) {
        String wifi_command = String(doc["wifi"].as<const char*>());
        Serial.println("通过" + via + "接收到WiFi命令: " + wifi_command);
        if (wifi_command == "on") {
            wifi_state = "on";
            setupWiFi();
            if (WiFi.status() == WL_CONNECTED) {
                sendMessage("{\"wifi\": \"true\"}", via);
            } else {
                sendMessage("{\"wifi\": \"false\"}", via);
            }
        } else if (wifi_command == "off") {
            wifi_state = "off";
            WiFi.disconnect();
            client.disconnect();
            mqtt_connected = false;  // 更新MQTT连接状态
            saveConfig();
        }
    } else if (!doc["ssid"].isNull() && via == "ble") {
        temp_ssid = String(doc["ssid"].as<const char*>());
        Serial.println("通过" + via + "接收到WiFi名称: " + temp_ssid);
    } else if (!doc["pw"].isNull() && via == "ble") {
        temp_password = String(doc["pw"].as<const char*>());
        Serial.println("通过" + via + "接收到WiFi密码: " + temp_password);
    } else if (!doc["type"].isNull()) {
        String type = String(doc["type"].as<const char*>());
        if (type == "try") {
            tryConnectWiFi(via);
        } else if (type == "get" && (via == "ble" || via == "mqtt")) {
            sendDeviceState(via);
        }
    }
}

// 尝试使用临时凭证连接WiFi
void tryConnectWiFi(const String& via) {
    if (temp_ssid.length() > 0 && temp_password.length() > 0) {
        Serial.println("尝试连接WiFi: " + temp_ssid);
        WiFi.begin(temp_ssid.c_str(), temp_password.c_str());
        unsigned long startMillis = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - startMillis > WIFI_CONNECT_TIMEOUT) {
                Serial.println("WiFi连接失败");
                sendMessage("{\"wifi\": \"false\"}", via);
                // 如果wifi_state为"on"，尝试连接之前保存的WiFi
                if (wifi_state == "on" && ssid.length() > 0 && password.length() > 0) {
                    Serial.println("尝试连接之前保存的WiFi: " + ssid);
                    WiFi.begin(ssid.c_str(), password.c_str());
                    startMillis = millis();
                    while (WiFi.status() != WL_CONNECTED) {
                        if (millis() - startMillis > WIFI_CONNECT_TIMEOUT) {
                            Serial.println("之前保存的WiFi连接失败");
                            wifi_state = "off";
                            saveConfig();
                            return;
                        }
                        esp_task_wdt_reset();  // 喂狗
                        delay(100);
                    }
                    Serial.println("之前保存的WiFi已连接");
                    wifi_state = "on";
                    saveConfig();
                    // MQTT连接将在mqttTask中处理
                }
                return;
            }
            esp_task_wdt_reset();  // 喂狗
            delay(100);
        }
        Serial.println("WiFi已连接");
        ssid = temp_ssid;
        password = temp_password;
        wifi_state = "on";  // 修改为"on"
        saveConfig();
        sendMessage("{\"wifi\": \"true\"}", via);
        // MQTT连接将在mqttTask中处理
    } else {
        Serial.println("未提供临时WiFi凭证");
        sendMessage("{\"wifi\": \"off\"}", via);
    }
}

// 根据亮度和色温设置两个灯的PWM占空比
void setLightsBrightness(int brightness, int color) {
    int warm, cool;
    int max_ratio = max(color, 100 - color);
    if (max_ratio == 0) {
        max_ratio = 1; // 避免除零错误
    }
    warm = (color / (float)max_ratio) * 100 * (brightness / 100.0);
    cool = ((100 - color) / (float)max_ratio) * 100 * (brightness / 100.0);

    // 将亮度值映射到PWM分辨率范围
    int warm_pwm = map(warm, 0, 100, 0, (1 << PWM_RESOLUTION) - 1);
    int cool_pwm = map(cool, 0, 100, 0, (1 << PWM_RESOLUTION) - 1);

    // 设置PWM占空比
    ledcWrite(PWM_CHANNEL_1, warm_pwm);
    ledcWrite(PWM_CHANNEL_2, cool_pwm);
}

// 发送消息（BLE和MQTT统一发送）
void sendMessage(const String& message, const String& via) {
    if (via == "ble" && ble_connected) {
        pNotifyCharacteristic->setValue(message.c_str());
        pNotifyCharacteristic->notify();
        Serial.println("通过BLE发送: " + message);
    } else if (via == "mqtt" && mqtt_connected) {
        String stateTopic = mqtt_topic + "/state";  // 发布到 /state 主题
        client.publish(stateTopic.c_str(), message.c_str());
        Serial.println("通过MQTT发送: " + message);
    } else {
        Serial.println("无活跃连接，无法发送消息: " + message);
    }
}