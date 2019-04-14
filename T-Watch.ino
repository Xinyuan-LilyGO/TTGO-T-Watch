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
#include <SPI.h>
#include <pcf8563.h>

/*********************
 *      DEFINES
 *********************/
#define TASK_QUEUE_MESSAGE_LEN      10
#define BACKLIGHT_CHANNEL           ((uint8_t)1)


/**********************
 *
 *  STATIC VARIABLES
 **********************/
AXP20X_Class axp;
PCF8563_Class rtc;
QueueHandle_t g_event_queue_handle = NULL;
static Ticker *wifiTicker = nullptr;
Ticker btnTicker;
Button2 btn(USER_BUTTON);

static bool stime_init();
static void time_task(void *param);

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



bool syncRtcBySystemTime()
{
    struct tm info;
    time_t now;
    time(&now);
    localtime_r(&now, &info);
    if (info.tm_year > (2016 - 1900)) {
        Serial.println("syncRtcBySystemTime");
        //Month (starting from January, 0 for January) - Value range is [0,11]
        rtc.setDateTime(info.tm_year, info.tm_mon + 1, info.tm_mday, info.tm_hour, info.tm_min, info.tm_sec);
        return true;
    }
    return false;
}


void syncSystemTimeByRtc()
{
    struct tm t_tm;
    struct timeval val;
    RTC_Date dt = rtc.getDateTime();
    t_tm.tm_hour = dt.hour;
    t_tm.tm_min = dt.minute;
    t_tm.tm_sec = dt.second;
    t_tm.tm_year = dt.year - 1900;    //Year, whose value starts from 1900
    t_tm.tm_mon = dt.month - 1;       //Month (starting from January, 0 for January) - Value range is [0,11]
    t_tm.tm_mday = dt.day;
    val.tv_sec = mktime(&t_tm);
    val.tv_usec = 0;
    settimeofday(&val, NULL);
    Serial.print("get RTC DateTime:");
    Serial.println(rtc.formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
}


bool flag;


typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    double lat;
    double lng;
} s7xg_gps_t;


int waitForResp(const char *command, const char *resp, unsigned int timeout)
{
    int len = strlen(resp);
    int sum = 0;
    unsigned long timerStart, timerEnd;
    timerStart = millis();

    if (command) {
        Serial1.print(command);
    }
    while (1) {
        if (Serial1.available()) {
            char c = Serial1.read();
            Serial.write(c);
            sum = (c == resp[sum]) ? sum + 1 : 0;
            if (sum == len)
                break;
        }
        timerEnd = millis();
        if (timerEnd - timerStart > 1000 * timeout) {
            if (command) {
                Serial.printf("[%s]Time out\n", command);

            } else
                Serial.println("Time out");
            return -1;
        }
    }
    while (Serial1.available()) {
        Serial1.read();
    }
    return 0;
}

#define POSITIONING_DONE  "\n\r>> DMS UTC"
#define DEFALUT_ACK "Ok"
#define DEFALUT_TIMEOUT 5

void startGPS()
{
    waitForResp("sip reset", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("gps reset", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("gps set_level_shift on", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("gps set_start hot", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("gps set_satellite_system gps", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("gps set_positioning_cycle 1000", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("gps set_port_uplink 20", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("gps set_format_uplink ipso", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("gps set_mode manual", DEFALUT_ACK, DEFALUT_TIMEOUT);

    for (;;) {
        Serial1.print("gps get_data dms");
        if (Serial1.available()) {
            String r = Serial1.readString();
            Serial.println(r);
            if (0 == strncmp(POSITIONING_DONE, r.c_str(), strlen(POSITIONING_DONE))) {
                const char *data = r.c_str();
                s7xg_gps_t gps;
                char buff[512];
                char *p = NULL;
                int ret = 0;
                ret = sscanf(data, "%*[^(](%[^)]", buff);
                // Serial.printf("ret:%d %s\n", ret, buff);  // 2019/03/26 02:25:46
                ret = sscanf(buff, "%d/%d/%d %d:%d:%d",
                             &gps.year, &gps.month, &gps.day,
                             &gps.hour, &gps.minute, &gps.second);
                Serial.printf(
                    "date: %d/%d/%d  time:%d:%d:%d\n",
                    gps.year, gps.month, gps.day,
                    gps.hour, gps.minute, gps.second);
                p = strstr (data, ")");
                ret = sscanf(p, "%*[^(](%[^)]", buff);
                // Serial.printf("ret:%d %s\n", ret, buff);
                int a, b;
                float c;
                sscanf(buff, "%d*%d'%f\"", &a, &b, &c);
                Serial.printf("lat: %d-%d-%.2f\n", a, b, c);
                p = strstr (p + 1, ")");
                ret = sscanf(p, "%*[^(](%[^)]", buff);
                // Serial.printf("ret:%d %s\n", ret, buff);
                sscanf(buff, "%d*%d'%f\"", &a, &b, &c);
                Serial.printf("long: %d-%d-%.2f\n", a, b, c);
            }
        }
        delay(1500);
    }
}


// size_t printNumber(unsigned long n)
// {
//     char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
//     char *str = &buf[sizeof(buf) - 1];

//     *str = '\0';

//     // prevent crash if called with base == 1
//      uint8_t   base = 16;

//     do {
//         unsigned long m = n;
//         n /= base;
//         char c = m - base * n;
//         *--str = c < 10 ? c + '0' : c + 'A' - 10;
//     } while(n);

//     return write(str);
// }

size_t toHexString(const char *str, uint8_t *buffer, size_t size)
{
    if (!str)return 0;
    if (!buffer)return 0;
    size_t index = 0;
    int c = *str;
    while (c != '\0') {
        if (!index)
            snprintf((char *)buffer, size, "%x", c);
        else
            snprintf((char *)buffer, size, "%s%x", buffer, c);
        c = *(++str);
        index++;
    }
    return index;
}

// rf lora_tx_stop
// rf lora_rx_stop
// rf set_freq <xxx> 862000000 to 932000000 S76s    Ok
// rf set_freq <xxx> 137000000 to 525000000 S78s    Ok
// rf set_pwr <xxx> 2 to 20 dNm
// rf set_sf <spreadingFactor>  7,8,9,10,11,12      Ok
// rf save  Save p2p configuration parameters to EEPROM
// rf get_freq
// rf get_pwr
// rf get_sf
// #define PINGPONG_TX

void lora_PingPong()
{
#ifdef PINGPONG_TX
    waitForResp("rf lora_rx_start 11223344556677889900", DEFALUT_ACK, DEFALUT_TIMEOUT);
#else
    waitForResp("rf lora_tx_start 0 10 11223344556677889900", DEFALUT_ACK, DEFALUT_TIMEOUT);
#endif

    for (;;) {
        if (Serial1.available()) {
            String r = Serial1.readString();
            Serial.println(r);
        }
    }
}


void lora_test()
{
    waitForResp("mac set_ch_freq 0 868500000", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("mac set_ch_freq 1 868700000", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("mac set_ch_freq 2 868900000", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("mac set_deveui 9c65f9fffeabcd12", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("mac set_appeui 70B3D57ED001A936", DEFALUT_ACK, DEFALUT_TIMEOUT);
    waitForResp("mac set_appkey C1FE94B0F5F6A50E83015B3C45C933A9", DEFALUT_ACK, DEFALUT_TIMEOUT);

    while (1) {
        Serial.println("\nTry to Join LoRaWaln");
        waitForResp("mac join otaa", DEFALUT_ACK, DEFALUT_TIMEOUT);
        if (0 == waitForResp(NULL, "\n\r>> accepted", DEFALUT_TIMEOUT * 3)) {
            Serial.println("\nJoin OK");
            break;
        }
        Serial.println("\nJoin FAIL");
        delay(5000);
    }

    uint32_t i = 0;
    char buff[512];
    uint8_t txBuf[512];
    for (;;) {

        snprintf(buff, sizeof(buff), "hello world %d", i++);
        toHexString(buff, txBuf, 512);
        snprintf(buff, sizeof(buff), "mac tx ucnf 15 %s", txBuf);
        Serial.printf("Send message %s\n", buff);
        waitForResp(buff, DEFALUT_ACK, DEFALUT_TIMEOUT);
        // waitForResp("mac tx ucnf 15 4845494920574F524944", DEFALUT_ACK, DEFALUT_TIMEOUT);
        waitForResp(NULL, "\n\r>> tx_ok", DEFALUT_TIMEOUT * 2);
        delay(5000);
    }
}

void setup()
{
    Serial.begin(115200);

// #define S7XG_DEBUG

#if 0
    Wire1.begin(SEN_SDA, SEN_SCL);
    axp.begin(Wire1);
    axp.setPowerOutPut(AXP202_LDO2, AXP202_ON);

    backlight_init();
    backlight_adjust(255);

    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);
    display_init();

#endif

#if defined(S7XG_DEBUG)
    Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX );
    Wire1.begin(SEN_SDA, SEN_SCL);
    axp.begin(Wire1);
    axp.setLDO4Voltage(AXP202_LDO4_1800MV);
    axp.setPowerOutPut(AXP202_LDO4, AXP202_ON);

    // startGPS();
    // lora_test();
    // lora_PingPong();

    for (;;) {
        if (Serial1.available()) {
            String r = Serial1.readString();
            Serial.println(r);
        }
        if (Serial.available()) {
            String r = Serial.readString();
            Serial1.write(r.c_str());
        }
    }
#endif
// #define RTC_DEBUG
#if defined(RTC_DEBUG)
    Wire1.begin(SEN_SDA, SEN_SCL);

    axp.begin(Wire1);

    rtc.begin(Wire1);

    pinMode(RTC_INT, INPUT_PULLUP); //need change to rtc_pin

    attachInterrupt(RTC_INT, []() {
        Serial.println("RTC Alarm");
        flag = 1;
    }, FALLING);

    rtc.disableAlarm();

    Serial.print("STATUS2");

    Serial.println(rtc.status2(), HEX);

    rtc.setDateTime(2019, 4, 7, 9, 05, 50);

    Serial.printf("Alarm is %s\n", rtc.alarmActive() ? "Enable" : "Disable");

    rtc.setAlarmByMinutes(6);

    rtc.enableAlarm();

    Serial.print("STATUS2");

    Serial.println(rtc.status2(), HEX);

    Serial.printf("Alarm is %s\n", rtc.alarmActive() ? "Enable" : "Disable");

    for (;;) {
        Serial.print(rtc.formatDateTime());
        Serial.print("  0x");
        Serial.println(rtc.status2(), HEX);

        if (flag) {
            flag = 0;
            detachInterrupt(RTC_INT);
            rtc.resetAlarm();
        }
        delay(1000);
    }
#endif

#if 1
    g_event_queue_handle = xQueueCreate(TASK_QUEUE_MESSAGE_LEN, sizeof(task_event_data_t));

    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);

    lv_filesystem_init();

    display_init();

    Wire1.begin(SEN_SDA, SEN_SCL);

    axp.begin(Wire1);

    backlight_init();

    //BL Power
    axp.setPowerOutPut(AXP202_LDO2, AXP202_ON);

    //GPS Power
    // axp.setPowerOutPut(AXP202_LDO3, AXP202_ON);

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

    rtc.begin(Wire1);

    syncSystemTimeByRtc();

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

    pinMode(AXP202_INT, INPUT_PULLUP);
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

    xTaskCreate(time_task, "time", 2048, NULL, 20, NULL);

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
        configTzTime("CST-8", "pool.ntp.org");

        task_event_data_t event_data;
        event_data.type = MESS_EVENT_TIME;
        event_data.time.event = LVGL_TIME_SYNC;
        xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);

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

void time_handle(void *param)
{
    bool rslt;
    char time_buf[64];
    char date_buf[64];
    time_struct_t *p = (time_struct_t *)param;
    switch (p->event) {
        {
        case LVGL_TIME_UPDATE:
            if (lv_main_in()) {
                strftime(time_buf, sizeof(time_buf), "%H:%M", &(p->time));
                strftime(date_buf, sizeof(date_buf), "%a %d %b ", &(p->time));
                lv_main_time_update(time_buf, date_buf);
            }
            break;
        case LVGL_TIME_ALARM:
            break;
        case LVGL_TIME_SYNC:
            do {
                rslt = syncRtcBySystemTime();
            } while (!rslt);
            break;
        default:
            break;
        }
    }
}

void loop()
{
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
                time_handle(&event_data.time);
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

static void time_task(void *param)
{
    struct tm time;
    uint8_t prev_min = 0;
    task_event_data_t event_data;
    Serial.println("Time Task Create ...");
    // configTzTime("CST-8", "pool.ntp.org");
    for (;;) {
        if (getLocalTime(&time)) {
            event_data.type = MESS_EVENT_TIME;
            event_data.time.event = LVGL_TIME_UPDATE;
            event_data.time.time = time;
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

extern "C" const char *get_s7xg_model()
{
    return "s78G";
}
extern "C" const char *get_s7xg_ver()
{
    return "V1.08";
}
extern "C" const char *get_s7xg_join()
{
    return "unjoined";
}

void gps_power_on()
{
    axp.setLDO3Mode(1);
    axp.setPowerOutPut(AXP202_LDO3, AXP202_ON);
}

void gps_power_off()
{
    axp.setPowerOutPut(AXP202_LDO3, AXP202_OFF);
}

