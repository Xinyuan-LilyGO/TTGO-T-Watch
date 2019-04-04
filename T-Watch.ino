#include "board_def.h"
#include "lv_dirver.h"
#include "lv_filesys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "struct_def.h"
#include <WiFi.h>
#include <lvgl.h>
#include "lv_setting.h"
#include "lv_swatch.h"
#include "gps_task.h"
#include "motion_task.h"
#include <Ticker.h>
#include "axp20x.h"
#include <Button2.h>


/*********************
 *      DEFINES
 *********************/
#define TASK_QUEUE_MESSAGE_LEN  10
#define BACKLIGHT_CHANNEL   ((uint8_t)1)


/**********************
 *  STATIC VARIABLES
 **********************/
AXP20X_Class axp;

xQueueHandle g_event_queue_handle = NULL;
static Ticker *wifiTicker = nullptr;
Ticker btnTicker;
Button2 btn(USER_BUTTON);

static bool stime_init();

void wifi_event_setup()
{
    WiFiEventId_t eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.print("WiFi lost connection. Reason: ");
        Serial.println(info.disconnected.reason);
    }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

    eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.println("Completed scan for access points");
        int16_t len =  WiFi.scanComplete();
        for (int i = 0; i < len; ++i) {
            lv_wifi_list_add(WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
        }
        WiFi.scanDelete();
    }, WiFiEvent_t::SYSTEM_EVENT_SCAN_DONE);

    eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.println("Connected to access point");
    }, WiFiEvent_t::SYSTEM_EVENT_STA_CONNECTED);

    eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.print("Obtained IP address: ");
        Serial.println(WiFi.localIP());
        task_event_data_t event_data;
        event_data.type = MESS_EVENT_WIFI;
        event_data.wifi.event = LVGL_WIFI_CONFIG_CONNECT_SUCCESS;
        xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
    }, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
}

void setup()
{
    Serial.begin(115200);

#if 1
    g_event_queue_handle = xQueueCreate(TASK_QUEUE_MESSAGE_LEN, sizeof(task_event_data_t));

    display_init();

    Wire1.begin(SEN_SDA, SEN_SCL);

    axp.begin(Wire1);

    backlight_init();

    axp.setPowerOutPut(AXP202_LDO2, AXP202_ON);

    lv_create_ttgo();

    int level = backlight_getLevel();
    for (int level = 0; level < 255; level += 25) {
        backlight_adjust(level);
        delay(100);
    }

    axp.enableIRQ(AXP202_ALL_IRQ, AXP202_OFF);

    axp.adc1Enable(0xFF, AXP202_OFF);

    axp.adc2Enable(0xFF, AXP202_OFF);

    axp.adc1Enable(AXP202_BATT_VOL_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1, AXP202_ON);

    axp.enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_CHARGING_FINISHED_IRQ, AXP202_ON);

    axp.clearIRQ();

    wifi_event_setup();

    motion_task_init();

    gps_task_init();

    lv_main();

    btn.setLongClickHandler([](Button2 & b) {
        task_event_data_t event_data;
        event_data.type = MESS_EVENT_POWER;
        event_data.power.event = LVGL_POWER_ENTER_SLEEP;
        xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
    });

    btnTicker.attach_ms(30, [] {
        btn.loop();
    });

    attachInterrupt(AXP202_INT, [] {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        task_event_data_t event_data;
        event_data.type = MESS_EVENT_POWER;
        event_data.power.event = LVGL_POWER_IRQ;
        xQueueSendFromISR(g_event_queue_handle, &event_data, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken)
        {
            portYIELD_FROM_ISR ();
        }
    }, FALLING);


    if (axp.isChargeing()) {
        charging_anim_start();
    }
#else
    gps_task_init();

    // lv_filesystem_init();

    g_event_queue_handle = xQueueCreate(TASK_QUEUE_MESSAGE_LEN, sizeof(task_event_data_t));

    Wire1.begin(SEN_SDA, SEN_SCL);

    axp.begin(Wire1);

    axp.enableIRQ(AXP202_ALL_IRQ, AXP202_OFF);

    axp.adc1Enable(0xFF, AXP202_OFF);

    axp.adc2Enable(0xFF, AXP202_OFF);

    axp.adc1Enable(AXP202_BATT_VOL_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1, AXP202_ON);

    axp.enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_CHARGING_FINISHED_IRQ, AXP202_ON);

    axp.clearIRQ();

    display_init();

    wifi_event_setup();

    motion_task_init();

    lv_main();

    backlight_init();

    axp.setPowerOutPut(AXP202_LDO2, AXP202_ON);

    btn.setLongClickHandler([](Button2 & b) {
        task_event_data_t event_data;
        event_data.type = MESS_EVENT_POWER;
        event_data.power.event = LVGL_POWER_ENTER_SLEEP;
        xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
    });

    btnTicker.attach_ms(30, [] {
        btn.loop();
    });

// #define WIFI_SSID   "Xiaomi"
// #define WIFI_PASSWD "12345678"
//     WiFi.begin(WIFI_SSID, WIFI_PASSWD);
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.println("");
//     Serial.println("WiFi connected");

    attachInterrupt(AXP202_INT, [] {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        task_event_data_t event_data;
        event_data.type = MESS_EVENT_POWER;
        event_data.power.event = LVGL_POWER_IRQ;
        xQueueSendFromISR(g_event_queue_handle, &event_data, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken)
        {
            portYIELD_FROM_ISR ();
        }
    }, FALLING);
#endif
}


void wifi_handle(void *data)
{
    int16_t len;
    wifi_struct_t *p = (wifi_struct_t *)data;
    switch (p->event) {
    case LVGL_WIFI_CONFIG_SCAN:
        Serial.println("[WIFI] WIFI Scan Start ...");
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        len = WiFi.scanNetworks();
        Serial.printf("[WIFI] WIFI Scan Done ...Scan %d len\n", len);
        break;
    case LVGL_WIFI_CONFIG_SCAN_DONE:
        break;
    case LVGL_WIFI_CONFIG_CONNECT_SUCCESS:
        Serial.println("Connect Success");
        wifiTicker->detach();
        delete wifiTicker;
        lv_wifi_connect_pass();
        stime_init();
        break;
    case LVGL_WIFI_CONFIG_CONNECT_FAIL:
        Serial.println("Connect Fail,Power OFF WiFi");
        WiFi.disconnect(true);
        wifiTicker->detach();
        delete wifiTicker;
        lv_wifi_connect_fail();

        break;
    case LVGL_WIFI_CONFIG_TRY_CONNECT: {
        wifi_auth_t *auth = (wifi_auth_t *)p->ctx;
        Serial.printf("Recv ssid - %s pass: %s\n", auth->ssid, auth->password);
        wl_status_t ret = WiFi.begin(auth->ssid, auth->password);
        Serial.printf("begin wifi ret:%d\n", ret);
        wifiTicker = new Ticker();
        wifiTicker->once_ms(10000, [] {
            task_event_data_t event_data;
            event_data.type = MESS_EVENT_WIFI;
            event_data.wifi.event = LVGL_WIFI_CONFIG_CONNECT_FAIL;
            xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
        });
    }
    break;
    default:
        break;
    }
}

Ticker pwmTicker;
power_data_t data;

void power_handle(void *param)
{
    power_struct_t *p = (power_struct_t *)param;
    switch (p->event) {
    case LVGL_POWER_GET_MOINITOR:
        pwmTicker.attach_ms(1000, [] {
            data.vbus_vol = axp.getVbusVoltage();
            data.vbus_cur = axp.getVbusCurrent();
            data.batt_vol = axp.getBattVoltage();
            if (axp.isChargeing())
            {
                data.batt_cur = axp.getBattChargeCurrent();
            } else
            {
                data.batt_cur = axp.getBattDischargeCurrent();
            }
            lv_update_power_info(&data);
        });
        break;
    case LVGL_POWER_MOINITOR_STOP:
        pwmTicker.detach();
        break;
    case LVGL_POWER_IRQ:
        axp.readIRQ();
        if (axp.isVbusPlugInIRQ()) {
            charging_anim_start();
        }
        if (axp.isVbusRemoveIRQ()) {
            charging_anim_stop();
        }
        if (axp.isChargingDoneIRQ()) {
            charging_anim_stop();
        }
        if (axp.isPEKShortPressIRQ()) {
            if (isBacklightOn()) {
                backlight_off();
            } else {
                backlight_on();
                touch_timer_create();
            }
        }
        axp.clearIRQ();
        break;
    case LVGL_POWER_ENTER_SLEEP: {
        lv_create_ttgo();
        int level = backlight_getLevel();
        for (; level > 0; level -= 25) {
            backlight_adjust(level);
            delay(100);
        }
        display_off();
        axp.setPowerOutPut(AXP202_LDO2, AXP202_OFF);
        esp_sleep_enable_ext1_wakeup( ((uint64_t)(((uint64_t)1) << USER_BUTTON)), ESP_EXT1_WAKEUP_ALL_LOW);
        esp_deep_sleep_start();
    }
    break;
    default:
        break;
    }
}


void loop()
{
    char time_buf[64];
    char date_buf[64];
    task_event_data_t event_data;
    for (;;) {
        if (xQueueReceive(g_event_queue_handle, &event_data, portMAX_DELAY) == pdPASS) {
            switch (event_data.type) {
            case MESS_EVENT_GPS:
                gps_handle(&event_data.gps);
                break;
            case MESS_EVENT_FILE:
                file_handle(&event_data.file);
                break;
            case MESS_EVENT_VOLT:
                break;
            case MESS_EVENT_SPEK:
                break;
            case MESS_EVENT_WIFI:
                // Serial.println("MESS_EVENT_WIFI event");
                wifi_handle(&event_data.wifi);
                break;
            case MESS_EVENT_MOTI:
                // Serial.println("MESS_EVENT_MOTI event");
                motion_handle(&event_data.motion);
                break;
            case MESS_EVENT_TIME:
                // Serial.println("MESS_EVENT_TIME event");
                if (lv_main_in()) {
                    strftime(time_buf, sizeof(time_buf), "%H:%M", &event_data.time);
                    strftime(date_buf, sizeof(date_buf), "%a %d %b ", &event_data.time);
                    lv_main_time_update(time_buf, date_buf);

                    // task_event_data_t event_data;
                    // event_data.type = MESS_EVENT_MOTI;
                    // event_data.motion.event = LVGL_MOTION_GET_TEMP;
                    // xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
                }
                break;
            case MESS_EVENT_POWER:
                power_handle(&event_data.power);
                break;
            default:
                Serial.println("Error event");
                break;
            }
        }
    }
}

void time_task(void *param)
{
    struct tm time;
    uint8_t prev_min = 0;
    task_event_data_t event_data;
    Serial.println("Time Task Create ...");
    configTzTime("CST-8", "pool.ntp.org");
    for (;;) {
        if (getLocalTime(&time)) {
            event_data.type = MESS_EVENT_TIME;
            event_data.time = time;
            xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
        }
        delay(1000);
    }
}

static bool stime_init()
{
    static bool isInit = false;
    if (WiFi.status() != WL_CONNECTED ) {
        return false;
    }
    if (isInit)return true;
    isInit = true;
    xTaskCreate(time_task, "time", 2048, NULL, 20, NULL);
    return true;
}


extern "C" int get_batt_percentage()
{
    return axp.getBattPercentage();
}
extern "C" int get_ld1_status()
{
    return 1;
}
extern "C" int get_ld2_status()
{
    return axp.isLDO2Enable();
}
extern "C" int get_ld3_status()
{
    return axp.isLDO3Enable();
}
extern "C" int get_ld4_status()
{
    return axp.isLDO4Enable();
}
extern "C" int get_dc2_status()
{
    return axp.isDCDC2Enable();
}
extern "C" int get_dc3_status()
{
    return axp.isDCDC3Enable();
}
extern "C" const char *get_wifi_ssid()
{
    return WiFi.SSID() == "" ? "None" : WiFi.SSID().c_str();
}
extern "C" const char *get_wifi_rssi()
{
    return String(WiFi.RSSI()).c_str();
}
extern "C" const char *get_wifi_channel()
{
    return String(WiFi.channel()).c_str();
}
extern "C" const char *get_wifi_address()
{
    return WiFi.localIP().toString().c_str();
}
extern "C" const char *get_wifi_mac()
{
    return WiFi.macAddress().c_str();
}



