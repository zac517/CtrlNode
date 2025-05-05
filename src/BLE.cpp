#include "BLE.h"

// 相关 UUID
#define SERVICE_UUID "00001810-0000-1000-8000-00805f9b34fb"
#define WRITE_CHARACTERISTIC_UUID "00002a56-0000-1000-8000-00805f9b34fb"
#define NOTIFY_CHARACTERISTIC_UUID "00002a57-0000-1000-8000-00805f9b34fb"

// 用于发送消息的特征值
BLECharacteristic *pNotifyCharacteristic = nullptr;

// 接收消息的回调
void (*BLECallback)(const String &) = nullptr;

// BLE 连接状态
bool ble_connected = false;

// 连接和断开回调
class MyServerCallbacks : public BLEServerCallbacks
{
public:
    void onConnect(BLEServer *pServer) override
    {
        Serial.println("客户端通过 BLE 连接");
        ble_connected = true;
    }

    void onDisconnect(BLEServer *pServer) override
    {
        Serial.println("客户端从 BLE 断开");
        ble_connected = false;
        BLEDevice::startAdvertising();
        Serial.println("广播已重启");
    }
};

// 接收消息的回调
void MyCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0)
    {
        String message = String(rxValue.c_str());
        Serial.println("通过 BLE 接收到: " + message);
        if (BLECallback != nullptr)
            BLECallback(message);
    }
};

bool BLELibrary::isConnected()
{
    return ble_connected;
}

// 初始化 BLE 模块
void BLELibrary::init(const char *deviceName, void (*callback)(const String &))
{
    BLEDevice::init(deviceName);

    // 创建 BLE 服务器和服务
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pWriteCharacteristic = pService->createCharacteristic(
        WRITE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE);
    pNotifyCharacteristic = pService->createCharacteristic(
        NOTIFY_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY);
    pNotifyCharacteristic->addDescriptor(new BLE2902());
    pService->start();

    // 设置广播参数
    BLEAdvertisementData advertisementData;

    // 将MAC地址转换为字节数组
    char mac[6];
    deviceId = BLEDevice::getAddress().toString().c_str();
    sscanf(deviceId.c_str(), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
           &mac[0], &mac[1], &mac[2],
           &mac[3], &mac[4], &mac[5]);

    // 创建制造商数据结构
    std::string manufacturerData;
    manufacturerData += (char)0xFF; // 制造商特定数据标志
    manufacturerData += (char)0xFE; // 私有制造商ID（0xFFFE）
    for (int i = 0; i < 6; i++)
        manufacturerData += mac[i]; // 添加MAC地址
    advertisementData.setManufacturerData(manufacturerData);
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();

    // 设置回调函数
    pWriteCharacteristic->setCallbacks(new MyCallbacks());
    BLECallback = callback;

    Serial.println("BLE 已初始化，等待客户端连接...");
    Serial.print("蓝牙 MAC 地址: " + deviceId + "\n");
}

// 发送消息
void BLELibrary::sendMessage(const String &message)
{
    pNotifyCharacteristic->setValue(message.c_str());
    pNotifyCharacteristic->notify();
    Serial.println("通过 BLE 发送: " + message);
}

BLELibrary BLE;