#ifndef WIFI_LIB_H
#define WIFI_LIB_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiLibrary {
private:
    WiFiLibrary(); // 私有构造函数，防止外部直接创建
    static WiFiLibrary* instance;

public:
    static WiFiLibrary* getInstance(); // 单例访问方法
    
    void connect(const String& ssid, const String& password);
    void disconnect();
    bool isConnected();
};

extern WiFiLibrary* WiFiLib; // 声明为指针

#endif