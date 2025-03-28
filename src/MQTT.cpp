#include "MQTT.h"

WiFiClient espClient;
PubSubClient client(espClient);
String topic;

void mqttTask(void* pvParameters) {
    WiFiLibrary* wifi = WiFiLibrary::getInstance();
    while (true) {
        if (wifi->isConnected()) {
            if (!client.connected()) {
                String clientId = "ESP32Client-" + String(random(0xffff), HEX);
                Serial.println("尝试MQTT连接...");
                if (client.connect(clientId.c_str())) {
                    client.subscribe((topic + "/control").c_str());
                    Serial.println("MQTT已连接");
                } else {
                    Serial.print("失败，rc=");
                    Serial.println(client.state());
                }
                client.subscribe((topic + "/control").c_str());
            }
            client.loop();
        }
        esp_task_wdt_reset();
        delay(10);
    }
}

bool MQTTLibrary::init(const char* server, int port, const String& deviceId, MQTT_CALLBACK_SIGNATURE) {
    topic = "Lumina/devices/" + deviceId;
    Serial.print("MQTT主题: ");
    Serial.println(topic);
    client.setCallback(callback);
    client.setServer(server, port);
    xTaskCreatePinnedToCore(mqttTask, "MQTT Task", 4096, NULL, 1, NULL, 0);
    return true;
}

bool MQTTLibrary::sendMessage(const String& message) {
    client.publish((topic + "/state").c_str(), message.c_str());
    Serial.println("通过MQTT发送: " + message);
    return true;
}

MQTTLibrary MQTT;