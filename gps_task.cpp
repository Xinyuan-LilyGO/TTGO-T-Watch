#include "gps_task.h"
#include "board_def.h"
#include <Arduino.h>
#include <lvgl.h>
#include <TinyGPS++.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "struct_def.h"
#include "lv_swatch.h"
#include <s7xg.h>

#define M8N_MOUDLE

#if defined(UBOX_M8N_GPS)
#ifdef  M8N_MOUDLE
#define GPS_BANUD_RATE  9600
#else
#define GPS_BANUD_RATE  115200
#endif
#endif

extern void gps_power_on();
extern void gps_power_off();
extern void s7xg_power_on();
extern void s7xg_power_off();

static HardwareSerial hwSerial(1);
static EventGroupHandle_t gpsEventGroup = NULL;
static gps_struct_t gps_data;

#if defined(UBOX_M8N_GPS)
static TinyGPSPlus *gps = nullptr;
#elif defined(ACSIP_S7XG)
S7XG_Class s7xg;
#define GPS_BANUD_RATE  115200
#endif

#define BIT0        0x01
#define BIT1        0x02

#define UPDATE_TIME 1000

#if defined(UBOX_M8N_GPS)
static void gps_task(void *parameters)
{
    uint32_t timestamp = 0;

    uint32_t teststamp = 0;
    while (1) {
        if (xEventGroupWaitBits(gpsEventGroup, BIT0, pdFALSE, pdTRUE, portMAX_DELAY) == BIT0) {
            switch (gps_data.event) {
            case LVGL_GPS_WAIT_FOR_DATA:
                if (millis() - teststamp  >  UPDATE_TIME) {
                    teststamp = millis();
                    Serial.println("[GPS] Get GPS Data ..");
                }
                if (gps->location.isValid()) {
                    gps_data.event = LVGL_GPS_DATA_READY;
                    Serial.println("[GPS] GPS Data isValid ***");
                    gps_anim_close();
                    gps_create_static_text();
                }
                break;
            case LVGL_GPS_DATA_READY:
                // if (!gps->location.isValid()) {
                // gps_data.event = LVGL_GPS_WAIT_FOR_DATA;
                // break;
                // }
                if (millis() - timestamp  >  UPDATE_TIME) {
                    timestamp = millis();
                    Serial.println("[GPS] update gps info ...");
                    gps_data.lng = gps->location.lng();
                    gps_data.lat = gps->location.lat();
                    gps_data.speed = gps->speed.kmph();
                    gps_data.altitude = gps->altitude.meters();
                    gps_data.course = gps->course.deg();
                    gps_data.satellites = gps->satellites.value();

                    gps_data.date.year = gps->date.year();
                    gps_data.date.month = gps->date.month();
                    gps_data.date.day = gps->date.day();
                    gps_data.date.hour = gps->time.hour();
                    gps_data.date.min = gps->time.minute();
                    gps_data.date.sec = gps->time.second();
                    lv_gps_static_text_update(&gps_data);
                }
                break;
            default:
                break;
            }
            while (hwSerial.available()) {
                gps->encode(hwSerial.read());
                // Serial.write(hwSerial.read());
            }
            // Serial.println();
            delay(200);
        }
    }
    delete gps;
    vTaskDelete(NULL);
}

void gps_handle(void *data)
{
    gps_struct_t *p = (gps_struct_t *)data;
    switch (p->event) {
    case LVGL_GPS_START:
        //Turn on gps power
        gps_power_on();
        gps_data.event = LVGL_GPS_WAIT_FOR_DATA;
#ifndef  M8N_MOUDLE
        hwSerial.printf("@GPPS<1>\r\n");
        delay(1000);
        hwSerial.printf("@GSP\r\n");
        delay(1000);
#endif
        xEventGroupSetBits(gpsEventGroup, BIT0);
        Serial.println("[GPS] gps power on ...");
        break;
    case LVGL_GPS_STOP:
        Serial.println("[GPS] gps power off ...");
        xEventGroupClearBits(gpsEventGroup, BIT0);
        gps_power_off();
        //Shutdown gps power
        gps_data.event = LVGL_GPS_STOP;
        break;
    case LVGL_GPS_DATA_READY:
        break;
    default:
        break;
    }
}
#elif defined(ACSIP_S7XG)

void s7xg_handle( void *param)
{
    lora_struct_t *p = (lora_struct_t *)param;
    switch (p->event) {
        {
        case LVGL_S7XG_GPS_START:
            s7xg.gpsReset();
            s7xg.gpsSetLevelShift(true);
            s7xg.gpsSetStart();
            s7xg.gpsSetSystem(0);
            s7xg.gpsSetPositioningCycle(1000);
            s7xg.gpsSetPortUplink(20);
            s7xg.gpsSetFormatUplink(1);
            s7xg.gpsSetMode(1);
            gps_data.event = LVGL_GPS_WAIT_FOR_DATA;
            xEventGroupSetBits(gpsEventGroup, BIT0);
            break;
        case LVGL_S7XG_GPS_STOP:
            gps_data.event = LVGL_GPS_STOP;
            xEventGroupClearBits(gpsEventGroup, BIT0);
            s7xg.gpsStop();
            break;
        case LVGL_S7XG_GPS_DATA_READY:
            break;
        case LVGL_S7XG_GPS_WAIT_FOR_DATA:
            break;
        case LVGL_S7XG_LORA_SEND:
            s7xg.loraSetPingPongSender();
            xEventGroupSetBits(gpsEventGroup, BIT1);
            break;
        case LVGL_S7XG_LORA_RECV:
            s7xg.loraSetPingPongReceiver();
            xEventGroupSetBits(gpsEventGroup, BIT1);
            break;
        case LVGL_S7XG_LORAWLAN:
            break;
        case LVGL_S7XG_LORA_STOP:
            s7xg.loraPingPongStop();
            xEventGroupClearBits(gpsEventGroup, BIT1);
            break;
        default:
            break;
        }
    }
}


static void s7xg_task(void *parameters)
{
    uint32_t timestamp = 0;
    uint32_t teststamp = 0;
    for (;;) {
        EventBits_t bits = xEventGroupWaitBits(gpsEventGroup, BIT0 | BIT1, pdFALSE, pdFALSE, portMAX_DELAY);
        if ( bits & BIT0) {
            switch (gps_data.event) {
            case LVGL_GPS_WAIT_FOR_DATA: {

                if (millis() - teststamp  >  UPDATE_TIME) {
                    teststamp = millis();
                    Serial.println("[GPS] Get GPS Data ..");
                }
                GPS_Class gps = s7xg.gpsGetData(GPS_DATA_TYPE_DD);
                if (gps.isVaild()) {
                    Serial.println("[GPS] GPS Data isValid ***");
                    gps_anim_close();
                    gps_create_static_text();
                    gps_data.event = LVGL_GPS_DATA_READY;
                }
            }
            break;
            case LVGL_GPS_DATA_READY:
                if (millis() - timestamp  >  UPDATE_TIME) {
                    timestamp = millis();
                    GPS_Class gps =  s7xg.gpsGetData(GPS_DATA_TYPE_DD);
                    if (gps.isVaild()) {
                        gps_data.lng = gps.lng();
                        gps_data.lat = gps.lat();
                        gps_data.date.year = gps.year();
                        gps_data.date.month = gps.month();
                        gps_data.date.day = gps.day();
                        gps_data.date.hour = gps.hour();
                        gps_data.date.min = gps.minute();
                        gps_data.date.sec = gps.second();
                        lv_gps_static_text_update(&gps_data);
                        // Serial.printf("%d-%d-%d --- %d:%d:%d -- %.2f - %.2f\n",
                        //               gps.year(), gps.month(), gps.day(),
                        //               gps.hour(), gps.minute(), gps.second(), gps.lat(), gps.lng()
                        //              );
                    }
                }
                break;
            default:
                break;
            }
        }

        if ( bits & BIT1) {
            lora_add_message(s7xg.loraGetPingPongMessage().c_str());
        }

        delay(1000);
    }
}

extern "C" const char *get_s7xg_model()
{
    String model = s7xg.getHardWareModel();
    return model == "" ? "N/A" : model.c_str();
}
extern "C" const char *get_s7xg_ver()
{
    String ver = s7xg.getVersion();
    return  ver == "" ? "N/A" : ver.c_str();
}
extern "C" const char *get_s7xg_join()
{
    return "unjoined";
}

#endif

void gps_task_init()
{
    gpsEventGroup = xEventGroupCreate();

    hwSerial.begin(GPS_BANUD_RATE, SERIAL_8N1, GPS_RX, GPS_TX);

#if defined(UBOX_M8N_GPS)
    gps = new TinyGPSPlus();

    xTaskCreatePinnedToCore(gps_task, "gps", 2048, NULL, 20, NULL, 0);
#elif defined(ACSIP_S7XG)

    s7xg.begin(hwSerial);

    //TODO
    //....

    s7xg_power_on();

    s7xg.reset();

    s7xg.gpsReset();

    xTaskCreatePinnedToCore(s7xg_task, "gps", 2048, NULL, 20, NULL, 0);
#endif
}