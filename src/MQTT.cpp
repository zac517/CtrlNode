#include "MQTT.h"

WiFiClient espClient;
PubSubClient client(espClient);
String topic;

void mqttTask(void* pvParameters) {
    while (true) {
        if (WiFiLib.isConnected()) {
            if (!client.connected()) {
                String clientId = "ESP32Client-" + String(random(0xffff), HEX);
                Serial.println("尝试 MQTT 连接...");
                if (client.connect(clientId.c_str())) {
                    client.subscribe((topic + "/control").c_str());
                    Serial.println("MQTT 已连接");
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

void MQTTLibrary::init(const char* server, int port, const String& deviceId, MQTT_CALLBACK_SIGNATURE) {
    topic = "Lumina/devices/" + deviceId;
    Serial.print("MQTT主题: ");
    Serial.println(topic);
    client.setCallback(callback);
    client.setServer(server, port);
    xTaskCreatePinnedToCore(mqttTask, "MQTT Task", 4096, NULL, 1, NULL, 0);
}

void MQTTLibrary::sendMessage(const String& message) {
    client.publish((topic + "/state").c_str(), message.c_str());
    Serial.println("通过 MQTT 发送: " + message);
}

MQTTLibrary MQTT;