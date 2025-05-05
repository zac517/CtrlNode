#include "MQTT.h"

WiFiClient espClient;
PubSubClient client(espClient);

// MQTT 服务器地址和端口
const char *mqtt_server = "broker-cn.emqx.io";
const int mqtt_port = 1883;
String topic;

// 遗嘱消息
const char *willMessage = "Device disconnected unexpectedly";
int willQoS = 1;
bool willRetain = false;

// 接收消息的回调
void (*MQTTCallback)(const String &) = nullptr;

// 接收消息的回调
void MyCallbacks(char *topic, byte *payload, unsigned int length)
{
    String message = "";
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    if (message == "ping")
        MQTT.sendMessage("pong");
    else
    {
        Serial.println("通过 MQTT 接收到: " + message);
        if (MQTTCallback != nullptr)
            MQTTCallback(message);
    }
}

// 检查 MQTT 连接状态
bool MQTTLibrary::isConnected()
{
    return client.connected();
}

// 连接 MQTT 服务器
void connect()
{
    if (!client.connected())
    {
        String clientId = "ESP32Client-" + String(random(0xffff), HEX);
        String willTopic = topic + "/state";
        if (client.connect(clientId.c_str(), willTopic.c_str(), willQoS, willRetain, willMessage) && client.subscribe((topic + "/control").c_str()))
        {
            Serial.println("MQTT 已连接");
        }
        else
        {
            Serial.print("失败，rc=");
            Serial.println(client.state());
        }
    }
}

void mqttTask(void *pvParameters)
{
    while (true)
    {
        if (WiFiLib.isConnected())
        {
            connect();
            client.loop();
        }
        esp_task_wdt_reset();
        delay(10);
    }
}

// 初始化 MQTT 模块
void MQTTLibrary::init(const String &deviceId, void (*cb)(const String &))
{
    // 设置 MQTT 服务器和端口
    client.setServer(mqtt_server, mqtt_port);
    xTaskCreatePinnedToCore(mqttTask, "MQTT Task", 4096, NULL, 1, NULL, 0);

    // 设置回调函数
    MQTT_CALLBACK_SIGNATURE = MyCallbacks;
    client.setCallback(callback);
    MQTTCallback = cb;

    topic = "Lumina/devices/" + deviceId;
    Serial.print("MQTT主题: " + topic);
}

// 发送消息
void MQTTLibrary::sendMessage(const String &message)
{
    client.publish((topic + "/state").c_str(), message.c_str());
    Serial.println("通过 MQTT 发送: " + message);
}

MQTTLibrary MQTT;