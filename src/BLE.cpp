#include "BLE.h"

#define SERVICE_UUID "00001810-0000-1000-8000-00805f9b34fb"
#define WRITE_CHARACTERISTIC_UUID "00002a56-0000-1000-8000-00805f9b34fb"
#define NOTIFY_CHARACTERISTIC_UUID "00002a57-0000-1000-8000-00805f9b34fb"

// 广播数据长度
#define MANUFACTURER_DATA_LEN 6

BLEServer* pServer = nullptr;
BLECharacteristic* pWriteCharacteristic = nullptr;
BLECharacteristic* pNotifyCharacteristic = nullptr;

bool ble_connected = false;

class MyServerCallbacks : public BLEServerCallbacks {
public:
    void onConnect(BLEServer* pServer) override {
        Serial.println("客户端通过BLE连接");
        ble_connected = true;
    }

    void onDisconnect(BLEServer* pServer) override {
        Serial.println("客户端从BLE断开");
        ble_connected = false;
        BLEDevice::startAdvertising();
        Serial.println("广播已重启");
    }
};

void BLELibrary::init(const char* deviceName, BLECharacteristicCallbacks* callbacks) {
    BLEDevice::init(deviceName);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService* pService = pServer->createService(SERVICE_UUID);

    pWriteCharacteristic = pService->createCharacteristic(
        WRITE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );

    pNotifyCharacteristic = pService->createCharacteristic(
        NOTIFY_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pNotifyCharacteristic->addDescriptor(new BLE2902());

    pService->start();

    

    BLEAdvertisementData advertisementData;
    
    // 将MAC地址转换为字节数组
    uint8_t macBytes[MANUFACTURER_DATA_LEN];
    deviceId = BLEDevice::getAddress().toString().c_str();
    sscanf(deviceId.c_str(), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
            &macBytes[0], &macBytes[1], &macBytes[2],
            &macBytes[3], &macBytes[4], &macBytes[5]);

    // 创建制造商数据结构
    String manufacturerDataStr;
    manufacturerDataStr += (char)0xFF; // 制造商特定数据标志
    manufacturerDataStr += (char)0xFE; // 私有制造商ID（0xFFFE）
    for (int i = 0; i < MANUFACTURER_DATA_LEN; i++) {
        manufacturerDataStr += (char)macBytes[i];
    }
    const char* manufacturerData = manufacturerDataStr.c_str();
    advertisementData.setManufacturerData(manufacturerData);
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();
    Serial.println("BLE已初始化，等待客户端连接...");
    Serial.print("蓝牙MAC地址: ");
    Serial.println(deviceId);
    pWriteCharacteristic->setCallbacks(callbacks);
}

void BLELibrary::sendMessage(const String& message) {
    if (ble_connected) {
        pNotifyCharacteristic->setValue(message.c_str());
        pNotifyCharacteristic->notify();
        Serial.println("通过 BLE 发送: " + message);
    }
}

BLELibrary BLE;