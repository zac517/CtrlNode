// filepath: [WiFiLib.cpp](http://_vscodecontentref_/0)
#include "WiFiLib.h"
#include <WiFi.h>

#define WIFI_CONNECT_TIMEOUT 5000

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

WiFiLibrary WiFiLib;