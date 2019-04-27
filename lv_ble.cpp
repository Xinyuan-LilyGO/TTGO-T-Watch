#include <Arduino.h>
#include "BLEDevice.h"
#include "struct_def.h"
#include "lv_swatch.h"
#include "Ticker.h"


#define BLE_SEVERICE_TYPE_SOIL  0X01
#define BLE_SEVERICE_TYPE_RGB   0x02

#define SCAN_MAX_COUNT      10
#define CTRL_SERVICE_UUID                   "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CTRL_CHARACTERISTIC_UUID            "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SENSOR_SERVICE_UUID                 "4fafc301-1fb5-459e-8fcc-c5c9c331914b"
#define SENSOR_CHARACTERISTIC_UUID          "beb5483c-36e1-4688-b7f5-ea07361b26a8"


extern QueueHandle_t g_event_queue_handle ;
static BLEClient  *pClient = nullptr;
static BLEScan *pBLEScan = nullptr;
static BLERemoteCharacteristic *pRemoteCharacteristic = nullptr;
static BLERemoteDescriptor *pRemoteSensorDescriptor = nullptr;
static BLERemoteCharacteristic *pRemoteSensorCharacteristic = nullptr;
static BLEAdvertisedDevice ScanDevices[SCAN_MAX_COUNT];
static Ticker *bleTicker = nullptr;

static bool doConnect = false;
static bool connected = false;
static bool doScan = false;
static uint8_t ledstatus;
static void SensorNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
static int devicesCount = 0;

static void SensorNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    if (length == 12) {

        lv_soil_data_update(*(float *) & (pData[0]), *(float *) & (pData[4]), *(int *) & (pData[8]));

        // Serial.printf("Humidity:%.2f Temperature:%.2f soil:%d%%\n",
        //               *(float *) & (pData[0]),
        //               * (float *) & (pData[4]),
        //               * (int *) & (pData[8])
        //              );
    }
}

class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
    }
    void onDisconnect(BLEClient *pclient)
    {
        if (connected) {
            connected = false;
            Serial.println("onDisconnect");
            task_event_data_t event_data;
            event_data.type = MESS_EVENT_BLE;
            event_data.ble.event = LV_BLE_DISCONNECT;
            xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
        }

    }
};


static bool connectToServer(int index)
{
    Serial.print("Forming a connection to ");
    // Serial.println(ScanDevices[index].getAddress().toString.c_str());

    pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(&ScanDevices[index]);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    BLERemoteService *pRemoteSensorService = pClient->getService(SENSOR_SERVICE_UUID);
    if (pRemoteSensorService == nullptr) {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(SENSOR_SERVICE_UUID);
        return false;
    } else {
        Serial.print(" - Found our service");
        Serial.println(SENSOR_SERVICE_UUID);
        pRemoteSensorCharacteristic = pRemoteSensorService->getCharacteristic(SENSOR_CHARACTERISTIC_UUID);
        if (pRemoteSensorCharacteristic->canNotify())
            pRemoteSensorCharacteristic->registerForNotify(SensorNotifyCallback);

        pRemoteSensorDescriptor =  pRemoteSensorCharacteristic->getDescriptor(BLEUUID("2902"));
        if (pRemoteSensorDescriptor != nullptr) {
            Serial.print(" - Found our pRemoteSensorDescriptor");
            pRemoteSensorDescriptor->writeValue(1);
        }
    }
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService *pRemoteService = pClient->getService(BLEUUID(CTRL_SERVICE_UUID));
    if (pRemoteService == nullptr) {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(CTRL_SERVICE_UUID);
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CTRL_CHARACTERISTIC_UUID));
    if (pRemoteCharacteristic == nullptr) {
        Serial.print("Failed to find our characteristic UUID: ");
        Serial.println(CTRL_CHARACTERISTIC_UUID);
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if (pRemoteCharacteristic->canRead()) {
        uint8_t *pData = pRemoteCharacteristic->readRawData();
        if (pData) {
            // Serial.printf("pRemoteCharacteristic Read : %x\n", pData[0]);
            // ledstatus =  pData[0];
        }
    }
    connected = true;
    return true;
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class CustomAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
    /**
      * Called for each advertising BLE server.
      */
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        // Serial.print("BLE Advertised Device found: ");
        // Serial.println(advertisedDevice.toString().c_str());
        // BLE Testing
        // 3c:71:bf:89:05:fe
        // We have found a device, let us now see if it contains the service we are looking for.
        // if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(CTRL_SERVICE_UUID))) {
        //     BLEDevice::getScan()->stop();
        //     myDevice = new BLEAdvertisedDevice(advertisedDevice);
        //     doConnect = true;
        //     doScan = true;
        // }
        std::string name = advertisedDevice.getName();
        if (name != "" && devicesCount < SCAN_MAX_COUNT) {
            ScanDevices[devicesCount] = advertisedDevice;
            ++devicesCount;
        }
    }
};

extern "C" void soil_led_control()
{
    if (connected) {
        ledstatus = !ledstatus;
        pRemoteCharacteristic->writeValue(ledstatus);
    }
}

void ble_init()
{
    // BLEDevice::deinit("");
    BLEDevice::init("");
    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new CustomAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    // pBLEScan->start(5, false);
}

void ble_handle(void *arg)
{

    ble_struct_t *p = (ble_struct_t *)arg;
    switch ((p->event)) {
    case  LV_BLE_SCAN:
        /**
        * @brief Start scanning and block until scanning has been completed.
        * @param [in] duration The duration in seconds for which to scan.
        * @return The BLEScanResults.
        */
        Serial.println("BLE Scan Start...");
        bleTicker = new Ticker();
        bleTicker->once_ms(5000, [] {
            task_event_data_t event_data;
            event_data.type = MESS_EVENT_BLE;
            event_data.ble.event = LV_BLE_SCAN_DONE;
            xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
        });
        pBLEScan->start(5, false);
        // BLEDevice::getScan()->start(0);
        break;

    case LV_BLE_SCAN_DONE:
        if (bleTicker != nullptr) {
            delete bleTicker;
            bleTicker = nullptr;
        }
        if (devicesCount) {
            for (int i = 0; i < devicesCount; i++) {
                std::string name = ScanDevices[i].getName();
                lv_ble_device_list_add(name.c_str());
            }
        } else {
            lv_ble_device_list_add(NULL);
        }
        break;

    case LV_BLE_CONNECT:
        if (p->index >= 0) {
            if (connectToServer(p->index)) {
                lv_soil_test_create();
            } else {
                lv_ble_mbox_event("Device is not support");
            }
        }
        break;

    case LV_BLE_CONNECT_SUCCESS:
        break;

    case LV_BLE_DISCONNECT:
        Serial.println("LV_BLE_DISCONNECT");
        if (devicesCount) {
            devicesCount = 0;
            if (connected) {
                Serial.println("pClient->disconnect()...");
                pRemoteSensorDescriptor->writeValue(0);
                connected = false;
                pClient->disconnect();
            } else {
                Serial.println("lv_ble_mbox_event...");
                lv_ble_mbox_event("Device Disconnect");
            }
        }
        break;
    default:
        break;
    }
}