// filepath: [WiFiLib.cpp](http://_vscodecontentref_/0)
#include "WiFiLib.h"
#include <WiFi.h>

#define WIFI_CONNECT_TIMEOUT 5000

// 初始化静态成员
WiFiLibrary* WiFiLibrary::instance = nullptr;
// 全局指针而非对象
WiFiLibrary* WiFiLib = nullptr;

WiFiLibrary* WiFiLibrary::getInstance() {
    if (WiFiLib == nullptr) {
        WiFiLib = new WiFiLibrary();
    }
    return WiFiLib;
}

// 在构造函数中明确初始化WiFi
WiFiLibrary::WiFiLibrary() {
    WiFi.mode(WIFI_STA); // 明确设置为Station模式
}

void WiFiLibrary::connect(const String& ssid, const String& password) {
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
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi已连接，IP地址: " + WiFi.localIP().toString());
}


void WiFiLibrary::disconnect() {
    WiFi.disconnect();
    Serial.println("WiFi已断开");
}

bool WiFiLibrary::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}