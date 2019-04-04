// #include "lv_ble.h"
// #include <BLEDevice.h>
// #include <BLEUtils.h>
// #include <BLEServer.h>

// #define APPLICATION_NAME   "T-Watch"
// #define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// #define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// void startBLEServer()
// {
//     BLEDevice::init(APPLICATION_NAME);
//     BLEServer *pServer = BLEDevice::createServer();
//     BLEService *pService = pServer->createService(SERVICE_UUID);
//     BLECharacteristic *pCharacteristic = pService->createCharacteristic(
//             CHARACTERISTIC_UUID,
//             BLECharacteristic::PROPERTY_READ |
//             BLECharacteristic::PROPERTY_WRITE
//                                          );

//     pCharacteristic->setValue("Hello World says Neil");
//     pService->start();
//     // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
//     BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
//     pAdvertising->addServiceUUID(SERVICE_UUID);
//     pAdvertising->setScanResponse(true);
//     pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
//     pAdvertising->setMinPreferred(0x12);
//     BLEDevice::startAdvertising();
// }