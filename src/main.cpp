#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include "WiFiLib.h"
#include "COMM.h"

#define LED 32  // 定义第一个LED引脚
#define LED2 33 // 定义第二个LED引脚
#define PWM_CHANNEL_1 0
#define PWM_CHANNEL_2 1
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// 本地存储的键名
const char* stored_power = "power";  // 电源状态
const char* stored_wifi = "wifi";  // WiFi状态
const char* stored_ssid = "ssid";  // WiFi名称
const char* stored_password = "password";  // WiFi密码
const char* stored_mode = "mode";  // 模式
const char* stored_brightness = "brightness";  // 亮度
const char* stored_color = "color";  // 色温

Preferences prefs;  // 创建Preferences对象，用于本地存储
String power_state = "off";
String wifi_state = "off";
String ssid = "";
String password = "";
int mode = 0;
int brightness = 0;
int color = 0;
String temp_ssid = "";
String temp_password = "";

void sendDeviceState() {
    String powerMessage = "{\"power\": \"" + power_state + "\"}";
    String modeMessage = "{\"mode\": " + String(mode) + "}";
    String brightnessMessage = "{\"bn\": " + String(brightness) + "}";
    String colorMessage = "{\"color\": " + String(color) + "}";
    String wifiMessage = "{\"wifi\": \"" + wifi_state + "\"}";

    COMM.sendMessage(powerMessage);
    COMM.sendMessage(modeMessage);
    COMM.sendMessage(brightnessMessage);
    COMM.sendMessage(colorMessage);
    COMM.sendMessage(wifiMessage);
    if (wifi_state == "on") {
        COMM.sendMessage("{\"wifi\": \"" + String(WiFiLib.isConnected() ? "true" : "false") + "\"}");
    }
}

// 根据亮度和色温设置两个灯的 PWM 占空比
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

// 保存配置到 Preferences
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

// 从 Preferences 加载配置
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

void printConfig() {
    Serial.println("加载的配置:");
    Serial.println("电源状态: " + power_state);
    Serial.println("WiFi状态: " + wifi_state);
    Serial.println("WiFi名称: " + ssid);
    Serial.println("WiFi密码: " + password);
    Serial.println("模式: " + String(mode));
    Serial.println("亮度: " + String(brightness));
    Serial.println("色温: " + String(color));
}

// 根据模式设置亮度和色温
void setMode(int new_mode) {
    switch (new_mode) {
        case 0: // 均衡
            brightness = 70;
            color = 50;
            break;
        case 1: // 夜间
            brightness = 20;
            color = 80;
            break;
        case 2: // 专注
            brightness = 90;
            color = 20;
            break;
        default:
            break;
    }
    setLightsBrightness(brightness, color);
    saveConfig();
}

void handleReceivedMessage(const String& message) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Serial.print("JSON解析失败: ");
        Serial.println(error.c_str());
        return;
    }

    if (!doc["power"].isNull()) {
        power_state = String(doc["power"].as<const char*>());
        if (power_state == "on") setLightsBrightness(brightness, color);
        else setLightsBrightness(0, 0);    
    } else if (!doc["mode"].isNull()) {
        int new_mode = doc["mode"].as<int>();
        if (new_mode >= 0 && new_mode <= 2) {
            setMode(new_mode);
            mode = new_mode;
        }
    } else if (!doc["bn"].isNull()) {
        brightness = doc["bn"].as<int>();
        setLightsBrightness(brightness, color);
    } else if (!doc["color"].isNull()) {
        color = doc["color"].as<int>();
        setLightsBrightness(brightness, color);
    } else if (!doc["wifi"].isNull()) {
        String wifi_command = String(doc["wifi"].as<const char*>());
        if (wifi_command == "on") {
            wifi_state = "on";
            WiFiLib.connect(ssid, password);
            COMM.sendMessage("{\"wifi\": \"" + String(WiFiLib.isConnected() ? "true" : "false") + "\"}");
        } else if (wifi_command == "off") {
            wifi_state = "off";
            WiFiLib.disconnect();
        }
    } else if (!doc["ssid"].isNull()) {
        temp_ssid = String(doc["ssid"].as<const char*>());
    } else if (!doc["pw"].isNull()) {
        temp_password = String(doc["pw"].as<const char*>());
    } else if (!doc["type"].isNull()) {
        String type = String(doc["type"].as<const char*>());
        if (type == "try") {
            if (temp_ssid.length() > 0 && temp_password.length() > 0) {
                WiFiLib.connect(temp_ssid, temp_password);
                if (WiFiLib.isConnected()) {
                    ssid = temp_ssid;
                    password = temp_password;
                    wifi_state = "on";
                    COMM.sendMessage("{\"wifi\": \"true\"}");
                }
                else {
                    WiFiLib.connect(ssid, password);
                    COMM.sendMessage("{\"wifi\": \"false\"}");
                    Serial.println("WiFi连接失败，恢复原始凭证");
                }
                COMM.sendMessage("{\"wifi\": \"" + String(WiFiLib.isConnected() ? "true" : "false") + "\"}");
            } else {
                Serial.println("未提供临时WiFi凭证");
                COMM.sendMessage("{\"wifi\": \"" + String(WiFiLib.isConnected() ? "true" : "false") + "\"}");
            }
        } else if (type == "get") {
            sendDeviceState();
        }
    }

    saveConfig();
}

void setup() {
    Serial.begin(115200);

    ledcSetup(PWM_CHANNEL_1, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LED, PWM_CHANNEL_1);
    ledcSetup(PWM_CHANNEL_2, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LED2, PWM_CHANNEL_2);

    esp_task_wdt_init(30, false);
    esp_task_wdt_add(NULL);
    
    loadConfig();
    printConfig();

    if (power_state == "on") setLightsBrightness(brightness, color);
    else setLightsBrightness(0, 0);
    if (wifi_state == "on") WiFiLib.connect(ssid, password);

    COMM.init("Lumina 台灯", handleReceivedMessage);
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
        esp_task_wdt_reset();
    }
}    