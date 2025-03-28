#ifndef WIFI_LIB_H
#define WIFI_LIB_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiLibrary {
public: 
    void connect(const String& ssid, const String& password);
    void disconnect();
    bool isConnected();
};

extern WiFiLibrary WiFiLib;

#endif