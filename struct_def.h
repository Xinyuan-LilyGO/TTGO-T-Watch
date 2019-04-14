

#ifndef __STRUCT_DEF_H
#define __STRUCT_DEF_H
#include <Arduino.h>
#include <time.h>

typedef enum {
    DIRECTION_TOP_EDGE        = 0,    /* Top edge of the board points towards the ceiling */
    DIRECTION_BOTTOM_EDGE     = 1,    /* Bottom edge of the board points towards the ceiling */
    DIRECTION_LEFT_EDGE       = 2,    /* Left edge of the board (USB connector side) points towards the ceiling */
    DIRECTION_RIGHT_EDGE      = 3,    /* Right edge of the board points towards the ceiling */
    DIRECTION_DISP_UP         = 4,    /* Display faces up (towards the sky/ceiling) */
    DIRECTION_DISP_DOWN       = 5     /* Display faces down (towards the ground) */
} direction_t;

typedef enum {
    MESS_EVENT_GPS,
    MESS_EVENT_FILE,
    MESS_EVENT_VOLT,
    MESS_EVENT_SPEK,
    MESS_EVENT_WIFI,
    MESS_EVENT_MOTI,
    MESS_EVENT_TIME,
    MESS_EVENT_POWER,
    MESS_EVENT_LORA,
} message_type_t;

typedef enum {
    LVGL_WIFI_CONFIG_SCAN = 0,
    LVGL_WIFI_CONFIG_SCAN_DONE,
    LVGL_WIFI_CONFIG_CONNECT_SUCCESS,
    LVGL_WIFI_CONFIG_CONNECT_FAIL,
    LVGL_WIFI_CONFIG_TRY_CONNECT,
} wifi_config_event_t;

typedef enum {
    DIR_TYPE,
    FILE_TYPE,
} file_type_t;

typedef enum {
    LVGL_FILE_SCAN,
} file_event_t;

typedef enum {
    LVGL_GPS_START,
    LVGL_GPS_STOP,
    LVGL_GPS_DATA_READY,
    LVGL_GPS_WAIT_FOR_DATA,
} gps_event_t;

typedef enum {
    LVGL_MOTION_STOP,
    LVGL_MOTION_GET_ACCE,
    LVGL_MOTION_GET_STEP,
    LVGL_MOTION_GET_TEMP,
} motion_event_t;

typedef enum {
    LVGL_POWER_GET_MOINITOR,
    LVGL_POWER_MOINITOR_STOP,
    LVGL_POWER_IRQ,
    LVGL_POWER_ENTER_SLEEP,
} power_event_t;

typedef enum {
    LVGL_TIME_UPDATE,
    LVGL_TIME_ALARM,
    LVGL_TIME_SYNC,
} time_event_t;

typedef struct  {
    double lng;
    double lat;
    double speed;
    double altitude;
    struct gpsdate {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t min;
        uint8_t sec;
    } date;
    uint32_t time;
    uint32_t course;
    uint32_t satellites;
    int gpsVaild;
    gps_event_t event;
} gps_struct_t;

typedef struct {
    file_event_t event;
} file_struct_t;

struct  {

} speaker_struct_t;

typedef struct voltage_struct {

} voltage_struct_t;

typedef struct {
    char ssid[64];
    char password[64];
} wifi_auth_t;

typedef struct  {
    wifi_config_event_t event;
    void *ctx;
} wifi_struct_t;

typedef struct  {
    motion_event_t event;
    direction_t dir;
    uint32_t setp;
} motion_struct_t;

typedef struct {
    float vbus_vol;
    float vbus_cur;
    float batt_vol;
    float batt_cur;
    float power;
} power_data_t;

typedef struct {
    power_event_t event;
    power_data_t data;
} power_struct_t;

typedef struct {
    time_event_t event;
    struct tm time;
} time_struct_t;

typedef struct {
    union {
        gps_struct_t gps;
        wifi_struct_t wifi;
        motion_struct_t motion;
        time_struct_t time;
        file_struct_t file;
        power_struct_t power;
    };
    void *arg;
    message_type_t type;
} task_event_data_t;

#endif  //*__STRUCT_DEF_H*/