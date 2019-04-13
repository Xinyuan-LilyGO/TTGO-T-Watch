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

extern void gps_power_on();
extern void gps_power_off();

static TinyGPSPlus *gps = nullptr;
static HardwareSerial gpsSerial(1);
static gps_struct_t gps_data;
static EventGroupHandle_t gpsEventGroup = NULL;

#define BIT0        0x01
#define UPDATE_TIME 1000

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
                if (!gps->location.isValid()) {
                    gps_data.event = LVGL_GPS_WAIT_FOR_DATA;
                    break;
                }
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
            while (gpsSerial.available()) {
                // Serial.write(gpsSerial.read());
                // String str = gpsSerial.readString();
                // Serial.println(gpsSerial.read());
                gps->encode(gpsSerial.read());
            }
            Serial.println();
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
        // enableGPSPower(true);
        gps_power_on();
        gps_data.event = LVGL_GPS_WAIT_FOR_DATA;
        gpsSerial.printf("@GPPS<1>\r\n");
        delay(1000);
        gpsSerial.printf("@GSP\r\n");
        delay(1000);
        xEventGroupSetBits(gpsEventGroup, BIT0);
        Serial.println("[GPS] gps power on ...");
        break;
    case LVGL_GPS_STOP:
        Serial.println("[GPS] gps power off ...");
        xEventGroupClearBits(gpsEventGroup, BIT0);
        gps_power_off();
        //Shutdown gps power
        // enableGPSPower(false);

        gps_data.event = LVGL_GPS_STOP;
        break;
    case LVGL_GPS_DATA_READY:
        break;
    default:
        break;
    }
}

void gps_task_init()
{
    gps = new TinyGPSPlus();
    gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
    // gpsSerial.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);

    gpsEventGroup = xEventGroupCreate();

    xTaskCreatePinnedToCore(gps_task, "gps", 2048, NULL, 20, NULL, 0);
}