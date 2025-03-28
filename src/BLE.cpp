#include "BLE.h"

#define SERVICE_UUID "00001810-0000-1000-8000-00805f9b34fb"
#define WRITE_CHARACTERISTIC_UUID "00002a56-0000-1000-8000-00805f9b34fb"
#define NOTIFY_CHARACTERISTIC_UUID "00002a57-0000-1000-8000-00805f9b34fb"

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

bool BLELibrary::init(const char* deviceName, BLECharacteristicCallbacks* callbacks) {
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
    advertisementData.setManufacturerData("Lumina");
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();
    Serial.println("BLE已初始化，等待客户端连接...");
    deviceId = BLEDevice::getAddress().toString().c_str();
    Serial.print("蓝牙MAC地址: ");
    Serial.println(deviceId);
    pWriteCharacteristic->setCallbacks(callbacks);
    return true;
}

bool BLELibrary::sendMessage(const String& message) {
    if (ble_connected) {
        pNotifyCharacteristic->setValue(message.c_str());
        pNotifyCharacteristic->notify();
        Serial.println("通过BLE发送: " + message);
    }
    return true;
}

BLELibrary BLE;