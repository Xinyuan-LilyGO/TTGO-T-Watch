/**
 * @file lv_test_tileview.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#ifdef ESP32
#include <lvgl.h>
#include "lv_setting.h"
#include "struct_def.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "esp_wifi.h"

extern xQueueHandle g_event_queue_handle;
extern const char *get_wifi_channel();
extern const char *get_wifi_rssi();
extern const char *get_wifi_ssid();
extern const char *get_wifi_address();
extern const char *get_wifi_mac();
extern int get_batt_percentage();
extern int get_ld1_status();
extern int get_ld2_status();
extern int get_ld3_status();
extern int get_ld4_status();
extern int get_dc2_status();
extern int get_dc3_status();
extern const char *get_s7xg_model();
extern const char *get_s7xg_ver();
extern const char *get_s7xg_join();
#else

#include <lv_examples/lv_apps/lv_swatch/lv_swatch.h>

const char *get_wifi_channel()
{
    return "12";
}
const char *get_wifi_rssi()
{
    return "-90";
}
const char *get_wifi_ssid()
{
    return "Xiaomi";
}
const char *get_wifi_address()
{
    return "192.168.1.1";
}
const char *get_wifi_mac()
{
    return "ABC:DEF:GHI:JKL";
}

typedef struct {
    float vbus_vol;
    float vbus_cur;
    float batt_vol;
    float batt_cur;
    float power;
} power_data_t;


int get_batt_percentage()
{
    return 50;
}


int get_ld1_status()
{
    return 1;
} int get_ld2_status()
{
    return 1;
} int get_ld3_status()
{
    return 0;
} int get_ld4_status()
{
    return 1;
} int get_dc2_status()
{
    return 0;
} int get_dc3_status()
{
    return 1;
}

const char *get_s7xg_model()
{
    return "s78G";
}
const char *get_s7xg_ver()
{
    return "V1.08";
}
const char *get_s7xg_join()
{
    return "unjoined";
}


#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Create a tileview to test their functionalities
 */
LV_IMG_DECLARE(image_location);
LV_IMG_DECLARE(img_folder);
LV_IMG_DECLARE(img_placeholder);
LV_IMG_DECLARE(img_setting);
LV_IMG_DECLARE(img_wifi);
LV_IMG_DECLARE(img_folder);
LV_IMG_DECLARE(img_menu);
LV_IMG_DECLARE(img_desktop);
LV_IMG_DECLARE(img_directions);
LV_IMG_DECLARE(img_direction_up);
LV_IMG_DECLARE(img_direction_down);
LV_IMG_DECLARE(img_direction_right);
LV_IMG_DECLARE(img_direction_left);
LV_IMG_DECLARE(img_step_conut);
LV_IMG_DECLARE(img_temp);

LV_FONT_DECLARE(font_miami);
LV_FONT_DECLARE(font_miami_32);
LV_FONT_DECLARE(font_sumptuous);
LV_FONT_DECLARE(font_sumptuous_24);

LV_IMG_DECLARE(img_power);
LV_IMG_DECLARE(img_batt1);
LV_IMG_DECLARE(img_batt2);
LV_IMG_DECLARE(img_batt3);
LV_IMG_DECLARE(img_batt4);
LV_IMG_DECLARE(img_ttgo);

LV_IMG_DECLARE(img_ble);
LV_IMG_DECLARE(img_clock);
LV_IMG_DECLARE(img_calendar);
LV_IMG_DECLARE(img_lora);


typedef lv_res_t (*lv_menu_action_t) (lv_obj_t *obj);
typedef void (*lv_menu_destory_t) (void);

typedef struct {
    const char *name;
    lv_menu_action_t callback;
    lv_menu_destory_t destroy;
    void *src_img;
    lv_obj_t *cont;
} lv_menu_struct_t ;

typedef struct {
    const char *name;
    lv_obj_t *label;
} lv_gps_struct_t;

typedef struct {
    const char *name;
    lv_obj_t *label;
    const char *(*get_val)(void);
} lv_wifi_struct_t;

typedef struct {
    lv_obj_t *time_label;
    lv_obj_t *step_count_label;
    lv_obj_t *date_label;
    lv_obj_t *temp_label;
} lv_main_struct_t;

typedef struct {
    lv_obj_t *wifi_cont;
    lv_obj_t *scan_label;
    lv_obj_t *preload;
    lv_obj_t *wifilist;
    lv_obj_t *passkb;
} lv_wifi_scan_obj_t;


static lv_obj_t *menu_cont = NULL;
static lv_obj_t *main_cont = NULL;
static lv_obj_t *g_menu_win = NULL;
static lv_obj_t *setting_cont = NULL;
static lv_obj_t *file_cont = NULL;
static lv_obj_t *wifi_cont = NULL;
static lv_obj_t *g_tileview = NULL;
static int g_menu_view_width;
static int g_menu_view_height;
static int curr_index = -1;
static bool g_menu_in = false;
static int prev = -1;
#ifdef ESP32
static wifi_auth_t auth;
#endif

static lv_res_t lv_setting_backlight_action(lv_obj_t *obj, const char *txt);
static lv_res_t lv_setting_th_action(lv_obj_t *obj);
static lv_res_t lv_tileview_action(lv_obj_t *obj, lv_coord_t x, lv_coord_t y);
static lv_res_t menubtn_action(lv_obj_t *btn);

static lv_res_t lv_file_setting(lv_obj_t *par);
static lv_res_t lv_setting(lv_obj_t *par);
static lv_res_t lv_gps_setting(lv_obj_t *par);
static lv_res_t lv_wifi_setting(lv_obj_t *par);
static lv_res_t lv_motion_setting(lv_obj_t *par);
static lv_res_t lv_power_setting(lv_obj_t *par);

static void lv_gps_setting_destroy();
static void lv_wifi_setting_destroy();
static void lv_file_setting_destroy();
static void lv_motion_setting_destroy();
static void lv_connect_wifi(const char *password);
static void lv_power_setting_destroy(void);

static void lv_menu_del();
static const void *lv_get_batt_icon();

static lv_gps_struct_t gps_data[] = {
    {.name = "lat:"},
    {.name = "lng:"},
    {.name = "satellites:"},
    {.name = "date:"},
    {.name = "altitude:"},  //meters
    {.name = "course:"},
    {.name = "speed:"}      //kmph
};

static lv_wifi_struct_t wifi_data[] = {
    {.name = "SSID", .get_val = get_wifi_ssid},
    {.name = "IP", .get_val = get_wifi_address},
    {.name = "RSSI", .get_val = get_wifi_rssi},
    {.name = "CHL", .get_val = get_wifi_channel},
    {.name = "MAC", .get_val = get_wifi_mac},
};

static lv_res_t lv_bluetooth_setting(lv_obj_t *par);
static lv_res_t lv_clock_setting(lv_obj_t *par);
static lv_res_t lv_calendar_setting(lv_obj_t *par);
static void lv_bluetooth_setting_destroy(void);
static void lv_clock_setting_destroy(void);
static void lv_calendar_setting_destroy(void);


static lv_main_struct_t main_data;
static lv_wifi_scan_obj_t wifi_obj;

static lv_res_t lv_bluetooth_setting(lv_obj_t *par)
{

}
static lv_res_t lv_clock_setting(lv_obj_t *par)
{

}
static lv_res_t lv_calendar_setting(lv_obj_t *par)
{
    /*Create a Calendar object*/
    lv_obj_t *calendar = lv_calendar_create(par, NULL);
    // lv_obj_set_size(calendar, g_menu_view_width, g_menu_view_height);
    lv_obj_set_size(calendar, 240, 220);
    lv_obj_align(calendar, par, LV_ALIGN_OUT_TOP_MID, 0, -50);

    /*Create a style for the current week*/
    static lv_style_t style_week_box;
    lv_style_copy(&style_week_box, &lv_style_plain);
    style_week_box.body.border.width = 1;
    style_week_box.body.border.color = LV_COLOR_HEX3(0x333);
    style_week_box.body.empty = 1;
    style_week_box.body.radius = LV_RADIUS_CIRCLE;
    style_week_box.body.padding.ver = 3;
    style_week_box.body.padding.hor = 3;

    /*Create a style for today*/
    static lv_style_t style_today_box;
    lv_style_copy(&style_today_box, &lv_style_plain);
    style_today_box.body.border.width = 2;
    style_today_box.body.border.color = LV_COLOR_NAVY;
    style_today_box.body.empty = 1;
    style_today_box.body.radius = LV_RADIUS_CIRCLE;
    style_today_box.body.padding.ver = 3;
    style_today_box.body.padding.hor = 3;
    style_today_box.text.color = LV_COLOR_BLUE;

    /*Create a style for the highlighted days*/
    static lv_style_t style_highlighted_day;
    lv_style_copy(&style_highlighted_day, &lv_style_plain);
    style_highlighted_day.body.border.width = 2;
    style_highlighted_day.body.border.color = LV_COLOR_NAVY;
    style_highlighted_day.body.empty = 1;
    style_highlighted_day.body.radius = LV_RADIUS_CIRCLE;
    style_highlighted_day.body.padding.ver = 3;
    style_highlighted_day.body.padding.hor = 3;
    style_highlighted_day.text.color = LV_COLOR_BLUE;

    /*Apply the styles*/
    lv_calendar_set_style(calendar, LV_CALENDAR_STYLE_WEEK_BOX, &style_week_box);
    lv_calendar_set_style(calendar, LV_CALENDAR_STYLE_TODAY_BOX, &style_today_box);
    lv_calendar_set_style(calendar, LV_CALENDAR_STYLE_HIGHLIGHTED_DAYS, &style_highlighted_day);


    /*Set the today*/
    lv_calendar_date_t today;
    today.year = 2018;
    today.month = 10;
    today.day = 23;

    lv_calendar_set_today_date(calendar, &today);
    lv_calendar_set_showed_date(calendar, &today);

    /*Highlight some days*/
    static lv_calendar_date_t highlihted_days[3];       /*Only it's pointer will be saved so should be static*/
    highlihted_days[0].year = 2018;
    highlihted_days[0].month = 10;
    highlihted_days[0].day = 6;

    highlihted_days[1].year = 2018;
    highlihted_days[1].month = 10;
    highlihted_days[1].day = 11;

    highlihted_days[2].year = 2018;
    highlihted_days[2].month = 11;
    highlihted_days[2].day = 22;

    lv_calendar_set_highlighted_dates(calendar, highlihted_days, 3);
}

static void lv_bluetooth_setting_destroy(void)
{

}
static void lv_clock_setting_destroy(void)
{

}
static void lv_calendar_setting_destroy(void)
{

}

static lv_res_t lv_lora_setting(lv_obj_t *par)
{
    static lv_wifi_struct_t lora_data[] = {
        {.name = "Model", .get_val = get_s7xg_model},
        {.name = "Version", .get_val = get_s7xg_ver},
        {.name = "Join", .get_val = get_s7xg_join},
    };

    lv_obj_t *label = NULL;
    char buff[256];
    wifi_obj.wifi_cont = lv_obj_create(par, NULL);
    lv_obj_set_size(wifi_obj.wifi_cont,  g_menu_view_width, g_menu_view_height);
    lv_obj_set_style(wifi_obj.wifi_cont, &lv_style_transp_fit);
    for (int i = 0; i < sizeof(lora_data) / sizeof(lora_data[0]); ++i) {

        lora_data[i].label = lv_label_create(wifi_obj.wifi_cont, NULL);
        snprintf(buff, sizeof(buff), "%s:%s", lora_data[i].name, lora_data[i].get_val());
        lv_label_set_text(lora_data[i].label, buff);
        if (!i)
            lv_obj_align(lora_data[i].label, wifi_obj.wifi_cont, LV_ALIGN_IN_TOP_MID, 0, 0);
        else
            lv_obj_align(lora_data[i].label, lora_data[i - 1].label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    }
}

static void  lv_lora_setting_destroy(void)
{
    lv_obj_del(wifi_obj.wifi_cont);
}

static lv_menu_struct_t menu_data[]  = {
#if 01
    // {.name = "calendar", .callback = lv_calendar_setting, .destroy = lv_calendar_setting_destroy, .src_img = &img_calendar},
    // {.name = "ble", .callback = lv_bluetooth_setting, .destroy = lv_bluetooth_setting_destroy, .src_img = &img_ble},
    // {.name = "clock", .callback = lv_clock_setting, .destroy = lv_clock_setting_destroy, .src_img = &img_clock},
    {.name = "LoRa", .callback = lv_lora_setting, .destroy = lv_lora_setting_destroy, .src_img = &img_lora},
    {.name = "WiFi", .callback = lv_wifi_setting, .destroy = lv_wifi_setting_destroy, .src_img = &img_wifi},
    {.name = "Power", .callback = lv_power_setting, .destroy = lv_power_setting_destroy, .src_img = &img_power},
    {.name = "Setting", .callback = lv_setting, .destroy = NULL, .src_img = &img_setting},
    {.name = "SD", .callback = lv_file_setting, .destroy = lv_file_setting_destroy, .src_img = &img_folder},
    {.name = "GPS", .callback = lv_gps_setting, .destroy = lv_gps_setting_destroy, .src_img = &img_placeholder},
    {.name = "Sensor", .callback = lv_motion_setting, .destroy = lv_motion_setting_destroy, .src_img = &img_directions},
#else
    {.name = "WIFI", .callback = lv_wifi_setting, .destroy = lv_wifi_setting_destroy, .src_img = SYMBOL_WIFI},
    {.name = "Setting", .callback = lv_setting, .destroy = NULL, .src_img = SYMBOL_SETTINGS},
    {.name = "SD", .callback = lv_file_setting, .destroy = lv_file_setting_destroy, .src_img = SYMBOL_DIRECTORY},
    {.name = "GPS", .callback = lv_gps_setting, .destroy = lv_gps_setting_destroy, .src_img = SYMBOL_GPS},
    {.name = "monitor", .callback = lv_motion_setting, .destroy = lv_motion_setting_destroy, .src_img = SYMBOL_PLAY}
#endif
};




lv_res_t lv_timer_start(void (*timer_callback)(void *), uint32_t period, void *param)
{
    lv_task_t *handle =  lv_task_create(timer_callback, period, LV_TASK_PRIO_LOW, param);
    lv_task_once(handle);
    return LV_RES_OK;
}

static lv_obj_t *img_batt = NULL;
static lv_task_t *chaging_handle = NULL;
static uint8_t changin_icons = 0;
lv_task_t *monitor_handle = NULL;

static void monitor_callback(void *prarm)
{
    if (g_menu_in) {
        lv_img_set_src(img_batt, lv_get_batt_icon());
    }
}

static void batt_monitor_task()
{
    if (!monitor_handle)
        monitor_handle =  lv_task_create(monitor_callback, 5000, LV_TASK_PRIO_LOW, NULL);
}

static const void *lv_get_batt_icon()
{
    int percent = get_batt_percentage();
    if (percent > 98) {
        return &img_batt4;
    }
    if (percent > 80) {
        return &img_batt3;
    }
    if (percent > 50) {
        return &img_batt2;
    } else {
        return &img_batt1;
    }
}

void charging_anim_callback()
{
    static void *src_img[] = {
        &img_batt1,
        &img_batt2,
        &img_batt3,
        &img_batt4
    };
    if (g_menu_in) {
        changin_icons = changin_icons + 1 >= sizeof(src_img) / sizeof(src_img[0]) ? 0 : changin_icons + 1;
        lv_img_set_src(img_batt, src_img[changin_icons]);
    }
}

void charging_anim_stop()
{
    if (chaging_handle) {
        lv_task_del(chaging_handle);
        chaging_handle = NULL;
        changin_icons = 0;
        lv_img_set_src(img_batt, lv_get_batt_icon());
    }
}

void charging_anim_start()
{
    if (!chaging_handle) {
        chaging_handle =  lv_task_create(charging_anim_callback, 1000, LV_TASK_PRIO_LOW, NULL);
    }
}


static lv_point_t lv_font_get_size(lv_obj_t *obj)
{
    lv_style_t *style = lv_obj_get_style(obj);
    const lv_font_t *font = style->text.font;
    lv_label_ext_t *ext = lv_obj_get_ext_attr(obj);
    lv_point_t size;
    lv_txt_flag_t flag = LV_TXT_FLAG_NONE;
    if (ext->recolor != 0) flag |= LV_TXT_FLAG_RECOLOR;
    if (ext->expand != 0) flag |= LV_TXT_FLAG_EXPAND;
    lv_txt_get_size(&size, ext->text, font, style->text.letter_space, style->text.line_space, LV_COORD_MAX, flag);
    return size;
}
/*********************************************************************
 *
 *                          POWER
 *
 * ******************************************************************/
typedef struct {
    lv_obj_t *cont;
    lv_obj_t *img;
} lv_power_struct_t;

lv_power_struct_t pwm_data;

typedef struct {
    int base;
    int x;
    int y;
    lv_obj_t *label;
    lv_align_t align;
    const char *txt;
    int (*get_func)(void);
} lv_power_list_t;

lv_power_list_t list[] = {
    {-1, 10, 0, NULL, LV_ALIGN_IN_TOP_MID, "Batt"}, //0
    {0, -50, 0, NULL, LV_ALIGN_OUT_LEFT_MID, "USB"},  //1
    {0, 40, 0, NULL, LV_ALIGN_OUT_RIGHT_MID, "Uint"}, //2
    {0, 0, 10, NULL, LV_ALIGN_OUT_BOTTOM_MID, "0000.00"},  //3
    {1, 0, 10, NULL, LV_ALIGN_OUT_BOTTOM_MID, "0000.00"},  //4
    {2, 0, 10, NULL, LV_ALIGN_OUT_BOTTOM_MID, "mV"},  //5
    {3, 0, 10, NULL, LV_ALIGN_OUT_BOTTOM_MID, "0000.00"},  //6
    {4, 0, 10, NULL, LV_ALIGN_OUT_BOTTOM_MID, "0000.00"},  //7
    {5, 0, 10, NULL, LV_ALIGN_OUT_BOTTOM_MID, "mA"},  //8
};

enum {
    LV_BATT_VOL_INDEX = 3,
    LV_VBUS_VOL_INDEX = 4,
    LV_BATT_CUR_INDEX = 6,
    LV_VBUS_CUR_INDEX = 7,
};

void lv_update_power_info(power_data_t *data)
{
    char buff[128];
    snprintf(buff, sizeof(buff), "%.2f", data->vbus_vol);
    lv_label_set_text(list[LV_VBUS_VOL_INDEX].label, buff);

    snprintf(buff, sizeof(buff), "%.2f", data->vbus_cur);
    lv_label_set_text(list[LV_VBUS_CUR_INDEX].label, buff);

    snprintf(buff, sizeof(buff), "%.2f", data->batt_vol);
    lv_label_set_text(list[LV_BATT_VOL_INDEX].label, buff);

    snprintf(buff, sizeof(buff), "%.2f", data->batt_cur);
    lv_label_set_text(list[LV_BATT_CUR_INDEX].label, buff);
}

static lv_res_t lv_power_setting(lv_obj_t *par)
{
    pwm_data.cont = lv_obj_create(g_menu_win, NULL);
    lv_obj_set_size(pwm_data.cont,  g_menu_view_width, g_menu_view_height);
    lv_obj_set_style(pwm_data.cont, &lv_style_transp_fit);

    static lv_style_t style_txt;
    lv_style_copy(&style_txt, &lv_style_plain);
    style_txt.text.font = &lv_font_dejavu_20;
    style_txt.text.letter_space = 2;
    style_txt.text.line_space = 1;
    style_txt.text.color = LV_COLOR_HEX(0xffffff);

    for (int i = 0; i < sizeof(list) / sizeof(list[0]); i++) {
        list[i].label = lv_label_create(pwm_data.cont, NULL);
        lv_label_set_text(list[i].label, list[i].txt);
        if ( list[i].base != -1)
            lv_obj_align( list[i].label, list[list[i].base].label, list[i].align, list[i].x, list[i].y);
        else
            lv_obj_align( list[i].label, pwm_data.cont, LV_ALIGN_IN_TOP_MID, list[i].x, list[i].y);
        lv_obj_set_style(list[i].label, &style_txt);
    }

    lv_power_list_t state[] = {
        {-1, 0, 85, NULL, LV_ALIGN_IN_TOP_MID, "LD2"},
        {-2, 0, 5, NULL, LV_ALIGN_OUT_BOTTOM_MID, NULL, .get_func = get_ld2_status},
        {0, -50, 0, NULL, LV_ALIGN_OUT_LEFT_MID, "LD1"},
        {-2, 0, 5, NULL, LV_ALIGN_OUT_BOTTOM_MID, NULL, .get_func = get_ld1_status},
        {0, 40, 0, NULL, LV_ALIGN_OUT_RIGHT_MID, "LD3"},
        {-2, 0, 5, NULL, LV_ALIGN_OUT_BOTTOM_MID, NULL, .get_func = get_ld3_status},

        {1, 0, 0, NULL, LV_ALIGN_OUT_BOTTOM_MID, "DC2"},
        {-2, 0, 5, NULL, LV_ALIGN_OUT_BOTTOM_MID, NULL, .get_func = get_dc2_status},
        {3, 0, 0, NULL, LV_ALIGN_OUT_BOTTOM_MID, "LD4"},
        {-2, 0, 5, NULL, LV_ALIGN_OUT_BOTTOM_MID, NULL, .get_func = get_ld4_status},
        {5, 0, 0, NULL, LV_ALIGN_OUT_BOTTOM_MID, "DC3"},
        {-2, 0, 5, NULL, LV_ALIGN_OUT_BOTTOM_MID, NULL, .get_func = get_dc3_status},
    };

    static lv_style_t style_led;
    lv_style_copy(&style_led, &lv_style_pretty_color);
    style_led.body.radius = LV_RADIUS_CIRCLE;
    style_led.body.main_color = LV_COLOR_GREEN;
    style_led.body.grad_color = LV_COLOR_GREEN;
    style_led.body.border.color = LV_COLOR_GREEN;
    style_led.body.border.width = 3;
    style_led.body.border.opa = LV_OPA_100;
    style_led.body.shadow.color = LV_COLOR_GREEN;
    style_led.body.shadow.width = 0;

    for (int i = 0; i < sizeof(state) / sizeof(state[0]); i++) {
        if (state[i].base == -2) {
            state[i].label  = lv_led_create(pwm_data.cont, NULL);
            lv_obj_set_size(state[i].label, 10, 10);
            lv_obj_set_style(state[i].label, &style_led);
            lv_obj_align( state[i].label, state[i - 1].label, state[i].align, state[i].x, state[i].y);

            if (state[i].get_func()) {
                lv_led_on(state[i].label);
            } else {
                lv_led_off(state[i].label);
            }

        } else {
            state[i].label = lv_label_create(pwm_data.cont, NULL);
            lv_label_set_text(state[i].label, state[i].txt);
            if ( state[i].base != -1)
                lv_obj_align( state[i].label, state[state[i].base].label, state[i].align, state[i].x, state[i].y);
            else
                lv_obj_align( state[i].label, pwm_data.cont, LV_ALIGN_IN_TOP_MID, state[i].x, state[i].y);
            lv_obj_set_style(state[i].label, &style_txt);
        }
    }

#ifdef ESP32
    task_event_data_t event_data;
    event_data.type = MESS_EVENT_POWER;
    event_data.power.event = LVGL_POWER_GET_MOINITOR;
    xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
#endif
    return LV_RES_OK;
}

static void lv_power_setting_destroy(void)
{
#ifdef ESP32
    task_event_data_t event_data;
    event_data.type = MESS_EVENT_POWER;
    event_data.power.event = LVGL_POWER_MOINITOR_STOP;
    xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
#endif
    lv_obj_del(pwm_data.cont);
}

/*********************************************************************
 *
 *                          MONITOR
 *
 * ******************************************************************/
typedef struct {
    lv_obj_t *motion_cont;
    lv_obj_t *img;
} lv_motion_struct_t;

lv_motion_struct_t motion_obj;

static void *motion_img_src[4] = {
    &img_direction_up,
    &img_direction_down,
    &img_direction_left,
    &img_direction_right,
};

void motion_dir_update(uint8_t index)
{
    if (index > sizeof(motion_img_src) / sizeof(motion_img_src[0]))
        return;
    lv_img_set_src(motion_obj.img, motion_img_src[index]);
    lv_obj_align(motion_obj.img, NULL, LV_ALIGN_CENTER, 0, 0);
}

static lv_res_t lv_motion_setting(lv_obj_t *par)
{
    motion_obj.motion_cont = lv_obj_create(g_menu_win, NULL);
    lv_obj_set_size(motion_obj.motion_cont,  g_menu_view_width, g_menu_view_height);
    lv_obj_set_style(motion_obj.motion_cont, &lv_style_transp_fit);

    motion_obj.img = lv_img_create(motion_obj.motion_cont, NULL);
    lv_img_set_src(motion_obj.img, motion_img_src[0]);
    lv_obj_align(motion_obj.img, NULL, LV_ALIGN_CENTER, 0, 0);

#ifdef ESP32
    task_event_data_t event_data;
    event_data.type = MESS_EVENT_MOTI;
    event_data.motion.event = LVGL_MOTION_GET_ACCE;
    xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
#endif
    return LV_RES_OK;
}

static void lv_motion_setting_destroy(void)
{
#ifdef ESP32
    task_event_data_t event_data;
    event_data.type = MESS_EVENT_MOTI;
    event_data.motion.event = LVGL_MOTION_STOP;
    xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
#endif
    lv_obj_del(motion_obj.motion_cont);
}

/*********************************************************************
 *
 *                          GPS
 *
 * ******************************************************************/
uint8_t lv_gps_static_text_update(void *data)
{
#ifdef ESP32
    gps_struct_t *gps = (gps_struct_t *)data;
    char buff[128];

    snprintf(buff, sizeof(buff), "%.2f", gps->lat);
    lv_label_set_text(gps_data[0].label, buff);

    snprintf(buff, sizeof(buff), "%.2f", gps->lng);
    lv_label_set_text(gps_data[1].label, buff);

    snprintf(buff, sizeof(buff), "%u", gps->satellites);
    lv_label_set_text(gps_data[2].label, buff);

    snprintf(buff, sizeof(buff), "%u-%u-%u %u:%u:%u",
             gps->date.year,
             gps->date.month,
             gps->date.day,
             gps->date.hour,
             gps->date.min,
             gps->date.sec
            );
    lv_label_set_text(gps_data[3].label, buff);

    snprintf(buff, sizeof(buff), "%.2f", gps->altitude);
    lv_label_set_text(gps_data[4].label, buff);

    snprintf(buff, sizeof(buff), "%u", gps->course);
    lv_label_set_text(gps_data[5].label, buff);

    snprintf(buff, sizeof(buff), "%.2f", gps->speed);
    lv_label_set_text(gps_data[6].label, buff);
#endif

}

static lv_res_t lv_gps_static_text(lv_obj_t *par)
{
    lv_obj_t *label;
    lv_point_t size;
    lv_coord_t x = lv_obj_get_width(par) / 2 - 30;
    lv_coord_t y = 0;
    lv_coord_t u_offset = 17;
    lv_coord_t setup = 1;

    lv_obj_t *cont = lv_obj_create(par, NULL);
    lv_obj_set_size(cont,  g_menu_view_width, g_menu_view_height);
    lv_obj_set_style(cont, &lv_style_transp_fit);

    for (int i = 0; i < sizeof(gps_data) / sizeof(gps_data[0]); ++i) {
        label = lv_label_create(cont, NULL);
        lv_label_set_text(label, gps_data[i].name);
        size = lv_font_get_size(label);
        size.x = x - size.x;
        lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, size.x, u_offset * setup);

        gps_data[i].label = lv_label_create(cont, NULL);
        lv_label_set_text(gps_data[i].label, "N/A");
        lv_obj_align(gps_data[i].label, NULL, LV_ALIGN_IN_TOP_LEFT, lv_obj_get_width(cont) / 2, u_offset * setup);
        ++setup;
    }

    return LV_RES_OK;
}

static bool gps_anim_started = false;
static lv_obj_t *gps_anim_cont = NULL;


void gps_anim_close()
{
    if (gps_anim_started) {
        gps_anim_started = false;
        lv_obj_del(gps_anim_cont);
    }
}

void gps_create_static_text()
{
    lv_gps_static_text(g_menu_win);
}

static void gps_static_text_call(void *param)
{
    lv_gps_static_text(param);
}

static lv_res_t lv_gps_anim_start(lv_obj_t *par)
{
    gps_anim_cont = lv_obj_create(par, NULL);
    lv_obj_set_size(gps_anim_cont, g_menu_view_width, g_menu_view_height);
    lv_obj_set_style(gps_anim_cont, &lv_style_transp_fit);

    lv_obj_t *img = lv_img_create(gps_anim_cont, NULL);
    lv_img_set_src(img, &image_location);
    lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);
    static lv_anim_t a;
    a.var = img;
    a.start = lv_obj_get_y(img);
    a.end = lv_obj_get_y(img) - 10;
    a.fp = (lv_anim_fp_t)lv_obj_set_y;
    a.path = lv_anim_path_linear;
    a.end_cb = NULL;
    a.act_time = -1000;                         /*Negative number to set a delay*/
    a.time = 400;                               /*Animate in 400 ms*/
    a.playback = 1;                             /*Make the animation backward too when it's ready*/
    a.playback_pause = 0;                       /*Wait before playback*/
    a.repeat = 1;                               /*Repeat the animation*/
    a.repeat_pause = 100;                       /*Wait before repeat*/
    lv_anim_create(&a);
    return LV_RES_OK;
}

static void lv_gps_setting_destroy(void)
{
#ifdef ESP32
    task_event_data_t event_data;
    event_data.type = MESS_EVENT_GPS;
    event_data.gps.event = LVGL_GPS_STOP;
    xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
#endif
}

static lv_res_t lv_gps_setting(lv_obj_t *par)
{
#ifdef ESP32
    task_event_data_t event_data;
    event_data.type = MESS_EVENT_GPS;
    event_data.gps.event = LVGL_GPS_START;
    xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
#endif
    gps_anim_started = true;
    lv_gps_anim_start(par);
    return LV_RES_OK;
}

/*********************************************************************
 *
 *                          KEYBOARD
 *
 * ******************************************************************/
static const char *btnm_mapplus[][23] = {
    {
        "a", "b", "c",   "\n",
        "d", "e", "f",   "\n",
        "g", "h", "i",   "\n",
        "\202"SYMBOL_OK, "Del", "Exit", "\202"SYMBOL_RIGHT, ""
    },
    {
        "j", "k", "l", "\n",
        "n", "m", "o",  "\n",
        "p", "q", "r",  "\n",
        "\202"SYMBOL_OK, "Del", "Exit", "\202"SYMBOL_RIGHT, ""
    },
    {
        "s", "t", "u",   "\n",
        "v", "w", "x", "\n",
        "y", "z", "\n",
        "\202"SYMBOL_OK, "Del", "Exit", "\202"SYMBOL_RIGHT, ""
    },
    {
        "A", "B", "C",  "\n",
        "D", "E", "F",   "\n",
        "G", "H", "I",  "\n",
        "\202"SYMBOL_OK, "Del", "Exit", "\202"SYMBOL_RIGHT, ""
    },
    {
        "J", "K", "L", "\n",
        "N", "M", "O",  "\n",
        "P", "Q", "R", "\n",
        "\202"SYMBOL_OK, "Del", "Exit", "\202"SYMBOL_RIGHT, ""
    },
    {
        "S", "T", "U",   "\n",
        "V", "W", "X",   "\n",
        "Y", "Z", "\n",
        "\202"SYMBOL_OK, "Del", "Exit", "\202"SYMBOL_RIGHT, ""
    },
    {
        "1", "2", "3",  "\n",
        "4", "5", "6",  "\n",
        "7", "8", "9",  "\n",
        "\202"SYMBOL_OK, "Del", "Exit", "\202"SYMBOL_RIGHT, ""
    },
    {
        "0", "+", "-",  "\n",
        "/", "*", "=",  "\n",
        "!", "?", "#",  "\n",
        "\202"SYMBOL_OK, "Del", "Exit", "\202"SYMBOL_RIGHT, ""
    },
    {
        "<", ">", "@",  "\n",
        "%", "$", "(",  "\n",
        ")", "{", "}",  "\n",
        "\202"SYMBOL_OK, "Del", "Exit", "\202"SYMBOL_RIGHT, ""
    },
    {
        "[", "]", ";",  "\n",
        "\"", "'", ".", "\n",
        ",", ":",  "\n",
        "\202"SYMBOL_OK, "Del", "Exit", "\202"SYMBOL_RIGHT, ""
    }
};

static lv_obj_t *passkeyboard = NULL;
static lv_obj_t *keyboard_cont = NULL;
static lv_obj_t *pass = NULL;
static lv_obj_t *wifi_connect_label = NULL;

static lv_res_t btnm_action(lv_obj_t *btnm, const char *txt)
{
    static int index = 0;
    lv_kb_ext_t *ext = lv_obj_get_ext_attr(passkeyboard);

    if (strcmp(txt, SYMBOL_OK) == 0) {
        printf("OK\n");
        printf(lv_ta_get_text(ext->ta));
        lv_obj_del(keyboard_cont);
        lv_connect_wifi(lv_ta_get_text(ext->ta));
        printf("\n");
    } else if (strcmp(txt, "Exit") == 0) {
        lv_obj_del(keyboard_cont);
        lv_obj_set_hidden(g_menu_win, false);
    } else if (strcmp(txt, SYMBOL_RIGHT) == 0) {
        index = index + 1 >= sizeof(btnm_mapplus) / sizeof(btnm_mapplus[0]) ? 0 : index + 1;
        lv_kb_set_map(passkeyboard, btnm_mapplus[index]);
    } else if (strcmp(txt, SYMBOL_LEFT) == 0) {
        index = index - 1 >= 0  ? index - 1 : sizeof(btnm_mapplus) / sizeof(btnm_mapplus[0]) - 1;
        lv_kb_set_map(passkeyboard, btnm_mapplus[index]);
    } else if (strcmp(txt, "Del") == 0)
        lv_ta_del_char(ext->ta);
    else {
        lv_ta_add_text(ext->ta, txt);
    }
    return LV_RES_OK; /*Return OK because the button matrix is not deleted*/
}

static void create_keyboard()
{
    lv_obj_set_hidden(g_menu_win, true);
    keyboard_cont = lv_cont_create(menu_cont, NULL);
    lv_obj_set_size(keyboard_cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style(keyboard_cont, &lv_style_transp_fit);

    pass = lv_ta_create(keyboard_cont, NULL);
    lv_obj_set_height(pass, 35);
    lv_ta_set_one_line(pass, true);
    lv_ta_set_pwd_mode(pass, true);
    lv_ta_set_text(pass, "");
    lv_obj_align(pass, keyboard_cont, LV_ALIGN_IN_TOP_MID, 0, 30);

    passkeyboard = lv_kb_create(keyboard_cont, NULL);
    lv_kb_set_map(passkeyboard, btnm_mapplus[0]);
    lv_obj_set_height(passkeyboard, LV_VER_RES / 3 * 2);
    lv_obj_align(passkeyboard, keyboard_cont, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_kb_set_ta(passkeyboard, pass);
    lv_btnm_set_action(passkeyboard, btnm_action);
}

static void lv_connect_wifi(const char *password)
{
    lv_obj_set_hidden(g_menu_win, false);
    lv_obj_clean(wifi_obj.wifi_cont);

    wifi_connect_label = lv_label_create(wifi_obj.wifi_cont, NULL);
    lv_label_set_text(wifi_connect_label, "Connecting...");
    lv_obj_align(wifi_connect_label, wifi_obj.wifi_cont, LV_ALIGN_CENTER, 0, 0);

#ifdef ESP32
    strlcpy(auth.password, password, sizeof(auth.password));
    task_event_data_t event_data;
    event_data.type = MESS_EVENT_WIFI;
    event_data.wifi.event = LVGL_WIFI_CONFIG_TRY_CONNECT;
    event_data.wifi.ctx = &auth;
    printf("ssid:%s password:%s \n", auth.ssid, auth.password);
    xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
#endif
}

static void lv_refs_wifi(void *param)
{
    lv_obj_set_hidden(g_menu_win, false);
    lv_obj_del(wifi_obj.wifi_cont);
    wifi_obj.wifilist = NULL;
    lv_wifi_setting(g_menu_win);
}

void lv_wifi_connect_fail()
{
    lv_label_set_text(wifi_connect_label, "Connect FAIL");
    lv_obj_align(wifi_connect_label, wifi_obj.wifi_cont, LV_ALIGN_CENTER, 0, -10);
    lv_timer_start(lv_refs_wifi, 800, NULL);
}

void lv_wifi_connect_pass()
{
    lv_label_set_text(wifi_connect_label, "Connect PASS");
    lv_obj_align(wifi_connect_label, wifi_obj.wifi_cont, LV_ALIGN_CENTER, 0, -10);
    lv_timer_start(lv_refs_wifi, 800, NULL);
}

/*********************************************************************
 *
 *                          WIFI
 *
 * ******************************************************************/
static lv_obj_t *wifi_scan_label = NULL;

static void lv_wifi_setting_destroy()
{
    printf("wifi setting destroy\n");
    lv_obj_del(wifi_obj.wifi_cont);
    wifi_obj.wifilist = NULL;
}

static lv_res_t wifiap_list_action(lv_obj_t *obj)
{
    const char *ssid = lv_list_get_btn_text(obj);
#ifdef ESP32
    strlcpy(auth.ssid, ssid, sizeof(auth.ssid));
    printf("auth ssid:%s\n", auth.ssid);
#else
    printf("auth ssid:%s\n", ssid);
#endif
    create_keyboard();
    return LV_RES_OK;
}


uint8_t lv_wifi_list_add(const char *ssid, int32_t rssi, uint8_t ch)
{
    if (!wifi_obj.wifilist) {
        lv_obj_clean(wifi_obj.wifi_cont);
        wifi_obj.wifilist = lv_list_create(wifi_obj.wifi_cont, NULL);
        lv_obj_set_size(wifi_obj.wifilist,  g_menu_view_width, g_menu_view_height);
        lv_obj_align(wifi_obj.wifilist, wifi_obj.wifi_cont, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style(wifi_obj.wifilist, &lv_style_transp_fit);

    }
    lv_list_add(wifi_obj.wifilist, SYMBOL_WIFI, ssid, wifiap_list_action);
    return LV_RES_OK;
}

static lv_res_t wifi_scan_btn_cb(struct _lv_obj_t *obj)
{
#ifdef ESP32
    task_event_data_t event_data;
    event_data.type = MESS_EVENT_WIFI;
    event_data.wifi.event = LVGL_WIFI_CONFIG_SCAN;
    xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
    lv_obj_clean(wifi_obj.wifi_cont);
    lv_obj_t *label = lv_label_create(wifi_obj.wifi_cont, NULL);
    lv_label_set_text(label, "Scaning...");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, -10);
#else
    const char *ssid[] = {"A", "B"};
    for (int i = 0; i < 2; i++) {
        lv_wifi_list_add(ssid[i], 0, 0);
    }
#endif
    return LV_RES_OK;
}

static lv_res_t lv_wifi_setting(lv_obj_t *par)
{
    lv_obj_t *label = NULL;
    char buff[256];
    wifi_obj.wifi_cont = lv_obj_create(par, NULL);
    lv_obj_set_size(wifi_obj.wifi_cont,  g_menu_view_width, g_menu_view_height);
    lv_obj_set_style(wifi_obj.wifi_cont, &lv_style_transp_fit);
    for (int i = 0; i < sizeof(wifi_data) / sizeof(wifi_data[0]); ++i) {

        wifi_data[i].label = lv_label_create(wifi_obj.wifi_cont, NULL);
        snprintf(buff, sizeof(buff), "%s:%s", wifi_data[i].name, wifi_data[i].get_val());
        lv_label_set_text(wifi_data[i].label, buff);
        if (!i)
            lv_obj_align(wifi_data[i].label, wifi_obj.wifi_cont, LV_ALIGN_IN_TOP_MID, 0, 0);
        else
            lv_obj_align(wifi_data[i].label, wifi_data[i - 1].label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    }

    lv_obj_t *scanbtn = lv_btn_create(wifi_obj.wifi_cont, NULL);
    lv_obj_align(scanbtn, wifi_obj.wifi_cont, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_obj_set_size(scanbtn, 100, 25);
    label = lv_label_create(scanbtn, NULL);
    lv_label_set_text(label, "Scan");
    lv_btn_set_action(scanbtn, LV_BTN_ACTION_PR, wifi_scan_btn_cb);
    return LV_RES_OK;
}



/*********************************************************************
 *
 *                          FILE
 *
 * ******************************************************************/
typedef struct {
    lv_obj_t *cont;
    lv_obj_t *list;
} lv_file_struct_t;

static lv_file_struct_t  file;

static void lv_file_setting_destroy(void)
{
    lv_obj_del(file.cont);
    memset(&file, 0, sizeof(file));
    printf("lv_file_setting_destroy\n");
}

void lv_file_list_add(const char *filename, uint8_t type)
{
    if (!filename) {
        lv_obj_t *label = lv_label_create(file.cont, NULL);
        lv_label_set_text(label, "SD Card not found");
        lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
    } else {
        if (!file.list) {
            file.list = lv_list_create(file.cont, NULL);
            lv_obj_set_size(file.list,  g_menu_view_width, g_menu_view_height);
            lv_obj_align(file.list, NULL, LV_ALIGN_CENTER, 0, 0);
            lv_style_t *style = lv_list_get_style(file.list, LV_LIST_STYLE_BTN_REL);
            style->body.padding.ver = 10;
            lv_list_set_style(file.list, LV_LIST_STYLE_BTN_REL, style);
        }
        if (file.list) {
            if (type)
                lv_list_add(file.list, SYMBOL_DIRECTORY, filename, NULL);
            else
                lv_list_add(file.list, SYMBOL_FILE, filename, NULL);
        }
    }
}

static lv_res_t lv_file_setting(lv_obj_t *par)
{
    file.cont = lv_cont_create(par, NULL);
    lv_obj_set_style(file.cont, &lv_style_transp_fit);
    lv_obj_set_size(file.cont,  g_menu_view_width, g_menu_view_height);
#ifdef ESP32
    task_event_data_t event_data;
    event_data.type = MESS_EVENT_FILE;
    event_data.file.event = LVGL_FILE_SCAN;
    xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
#else
    const char *buff[] = {"1.jpg", "2.png", "mono"};
    for (int i = 0; i < sizeof(buff) / sizeof(buff[0]); ++i) {
        lv_file_list_add(buff[i], 0);
    }
#endif
    return LV_RES_OK;
}


/*********************************************************************
 *
 *                          SETTING
 *
 * ******************************************************************/
static lv_res_t lv_setting(lv_obj_t *par)
{
    printf("Create lv_setting\n");
    //! backlight level
    setting_cont = lv_cont_create(par, NULL);
    lv_obj_set_size(setting_cont,  g_menu_view_width, g_menu_view_height);
    lv_obj_set_style(setting_cont, &lv_style_transp_fit);


    lv_obj_t *label;
    label = lv_label_create(setting_cont, NULL);
    lv_label_set_text(label, "BL:");
    lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 40);

    static const char *btnm_str[] = {"1", "2", "3", ""};
    lv_obj_t *btnm = lv_btnm_create(setting_cont, NULL);
    lv_obj_set_size(btnm, 150, 50);
    lv_btnm_set_map(btnm, btnm_str);
    lv_obj_align(btnm, label, LV_ALIGN_OUT_RIGHT_MID, 30, 0);
    lv_btnm_set_toggle(btnm, true, 3);
    lv_btnm_set_action(btnm, lv_setting_backlight_action);

    label = lv_label_create(setting_cont, NULL);
    lv_label_set_text(label, "TH:");
    lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 100);

    lv_obj_t *ddlist = lv_ddlist_create(setting_cont, NULL);
    lv_obj_align(ddlist, label, LV_ALIGN_OUT_RIGHT_MID, 30, 0);
    lv_ddlist_set_options(ddlist, "Alien\nNight\nMono\nNemo\nMaterial");
    lv_ddlist_set_fix_height(ddlist, LV_DPI);
    lv_ddlist_close(ddlist, true);
    lv_ddlist_set_selected(ddlist, 1);
    lv_ddlist_set_hor_fit(ddlist, false);
    lv_obj_set_width(ddlist,  150);
    lv_ddlist_set_action(ddlist, lv_setting_th_action);
    return LV_RES_OK;
}


/*********************************************************************
 *
 *                          MENU
 *
 * ******************************************************************/

static void lv_menu_del()
{
    if (curr_index != -1) {
        if (menu_data[curr_index].destroy != NULL) {
            menu_data[curr_index].destroy();
        }
        curr_index = -1;
    }
    lv_obj_del(menu_cont);
    g_menu_in = true;
    prev = -1;
}

static lv_res_t win_btn_click(lv_obj_t *btn)
{
    lv_menu_del();
    return LV_RES_OK;
}


void create_menu(lv_obj_t *par)
{
    lv_obj_t *label;
    lv_obj_t *img;

    if (!par) {
        par = lv_scr_act();
    }

    static lv_style_t style_txt;
    lv_style_copy(&style_txt, &lv_style_plain);
    style_txt.text.font = &lv_font_dejavu_20;
    style_txt.text.color = LV_COLOR_WHITE;
    style_txt.image.color = LV_COLOR_YELLOW;
    // style_txt.image.intense = LV_OPA_50;
    style_txt.body.main_color = LV_COLOR_GRAY;
    style_txt.body.border.color = LV_COLOR_GRAY;
    // style_txt.body.border.width = 10;
    // style_txt.body.border.part = LV_BORDER_NONE;
    // style_txt.body.padding.ver = 0;
    // style_txt.body.padding.hor = 0;
    // style_txt.body.padding.inner = 0;
    style_txt.body.opa = LV_OPA_10;

    menu_cont = lv_obj_create(par, NULL);
    lv_obj_set_size(menu_cont, lv_obj_get_width(par), lv_obj_get_height(par));

    lv_obj_t *wp = lv_img_create(menu_cont, NULL);
    lv_img_set_src(wp, &img_desktop);
    lv_obj_set_width(wp, LV_HOR_RES);
    lv_obj_set_protect(wp, LV_PROTECT_POS);

    g_menu_win = lv_win_create(menu_cont, NULL);
    lv_win_set_title(g_menu_win, "Menu");
    lv_win_set_sb_mode(g_menu_win, LV_SB_MODE_OFF);
    lv_win_set_layout(g_menu_win, LV_LAYOUT_PRETTY);

    static lv_win_ext_t *ext1 ;
    ext1 = lv_obj_get_ext_attr(g_menu_win);
    lv_coord_t height =  lv_obj_get_height(ext1->header);
    lv_obj_t *win_btn = lv_win_add_btn(g_menu_win, SYMBOL_LEFT, win_btn_click);
    lv_win_set_btn_size(g_menu_win, 45);

    lv_win_set_style(g_menu_win, LV_WIN_STYLE_HEADER, &style_txt);
    lv_win_set_style(g_menu_win, LV_WIN_STYLE_BG, &style_txt);

    static const lv_point_t vp[] = {
        {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0},
        {LV_COORD_MIN, LV_COORD_MIN}
    };

    g_tileview = lv_tileview_create(g_menu_win, NULL);
    lv_tileview_set_valid_positions(g_tileview, vp);

    lv_tileview_ext_t *ext = lv_obj_get_ext_attr(g_tileview);
    ext->anim_time = 50;
    ext->action = lv_tileview_action;
    lv_page_set_sb_mode(g_tileview, LV_SB_MODE_OFF);

    g_menu_view_width = lv_obj_get_width(g_menu_win) - 20;
    g_menu_view_height = lv_obj_get_height(g_menu_win) - height - 10;
    lv_obj_set_size(g_tileview, g_menu_view_width, g_menu_view_height);

    lv_obj_t *prev_obj = NULL;
    lv_obj_t *cur_obj = NULL;

    for (int i = 0; i < sizeof(menu_data) / sizeof(menu_data[0]); ++i) {
        cur_obj = lv_obj_create(g_tileview, NULL);
        lv_obj_set_size(cur_obj, lv_obj_get_width(g_tileview), lv_obj_get_height(g_tileview));
        lv_obj_set_style(cur_obj, &lv_style_transp_fit);
        lv_obj_align(cur_obj, prev_obj, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

        lv_tileview_add_element(cur_obj);

        img = lv_img_create(cur_obj, NULL);
        lv_img_set_src(img, menu_data[i].src_img);
        lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, -20);

        label = lv_label_create(cur_obj, NULL);
        lv_label_set_text(label, menu_data[i].name);
        lv_obj_align(label, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        if (prev_obj != cur_obj) {
            prev_obj = cur_obj;
        }
    }
    return;
}

bool lv_main_in()
{
    return g_menu_in;
}

void lv_main_step_counter_update(const char *step)
{
    lv_label_set_text(main_data.step_count_label, step);
}

void lv_main_temp_update( const char *temp)
{
    // lv_label_set_text(main_data.temp_label, temp);
}

void lv_main_time_update(const char *time, const char *date)
{
    lv_label_set_text(main_data.time_label, time);
    lv_label_set_text(main_data.date_label, date);
}



void lv_create_ttgo()
{
    if (!main_cont) {
        main_cont = lv_cont_create(lv_scr_act(), NULL);
        lv_obj_set_size(main_cont, LV_HOR_RES, LV_VER_RES);
    } else
        lv_obj_clean(main_cont);

    static lv_style_t style;
    lv_style_copy(&style, &lv_style_scr);
    style.body.main_color = LV_COLOR_BLACK;
    style.body.grad_color = LV_COLOR_BLACK;
    lv_obj_set_style(main_cont, &style);

    lv_obj_t *img = lv_img_create(main_cont, NULL);
    lv_img_set_src(img, &img_ttgo);
    lv_img_set_style(img, &style);
    lv_obj_align(img, main_cont, LV_ALIGN_CENTER, 0, 0);
}


void lv_main(void)
{
    g_menu_in = true;

    lv_theme_set_current(lv_theme_material_init(100, NULL));

    lv_font_add(&font_miami, &lv_font_dejavu_20);
    lv_font_add(&font_miami_32, &lv_font_dejavu_20);
    lv_font_add(&font_sumptuous, &lv_font_dejavu_20);
    lv_font_add(&font_sumptuous_24, &lv_font_dejavu_20);


    if (!main_cont) {
        main_cont = lv_cont_create(lv_scr_act(), NULL);
        lv_obj_set_size(main_cont, LV_HOR_RES, LV_VER_RES);
    } else
        lv_obj_clean(main_cont);

    lv_obj_t *wp = lv_img_create(main_cont, NULL);
    lv_img_set_src(wp, &img_desktop);
    lv_obj_set_width(wp, LV_HOR_RES);
    lv_obj_set_protect(wp, LV_PROTECT_POS);

    static lv_style_t style1;
    lv_style_copy(&style1, &lv_style_plain);
    style1.text.font = &font_miami;
    style1.text.color = LV_COLOR_WHITE;

    main_data.time_label = lv_label_create(main_cont, NULL);
    lv_label_set_text(main_data.time_label, "01:80");
    lv_label_set_style(main_data.time_label, &style1);
    lv_obj_align(main_data.time_label, NULL, LV_ALIGN_CENTER, 0, -80);


    static lv_style_t style11;
    lv_style_copy(&style11, &lv_style_plain);
    style11.text.font = &font_sumptuous;
    style11.text.color = LV_COLOR_WHITE;

    main_data.date_label = lv_label_create(main_cont, NULL);
    lv_label_set_text(main_data.date_label, "Wed 8 OCT");
    lv_label_set_style(main_data.date_label, &style11);
    lv_obj_align(main_data.date_label, main_data.time_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


    static lv_style_t style2;
    lv_style_copy(&style2, &lv_style_plain);
    style2.text.font = &font_sumptuous_24;
    style2.text.color = LV_COLOR_WHITE;

    lv_obj_t *img = lv_img_create(main_cont, NULL);
    lv_img_set_src(img, &img_step_conut);
    lv_obj_align(img, NULL, LV_ALIGN_IN_TOP_LEFT, 15, 120);

    main_data.step_count_label = lv_label_create(main_cont, NULL);
    lv_label_set_text(main_data.step_count_label, "0");
    lv_label_set_style(main_data.step_count_label, &style2);
    lv_obj_align(main_data.step_count_label, img, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    // main_data.temp_label = lv_label_create(main_cont, NULL);
    // lv_label_set_text(main_data.temp_label, "00.00");
    // lv_label_set_style(main_data.temp_label, &style2);
    // lv_obj_align(main_data.temp_label, img, LV_ALIGN_OUT_RIGHT_MID, 95, 0);

    // img = lv_img_create(main_cont, NULL);
    // lv_img_set_src(img, &img_temp);
    // lv_obj_align(img, main_data.temp_label, LV_ALIGN_OUT_RIGHT_MID, 0, -2);


    img_batt = lv_img_create(main_cont, NULL);
    lv_img_set_src(img_batt, lv_get_batt_icon());
    lv_obj_align(img_batt, main_cont, LV_ALIGN_IN_TOP_RIGHT, -10, 5);


    static lv_style_t style_pr;
    lv_style_copy(&style_pr, &lv_style_plain);
    style_pr.image.color = LV_COLOR_WHITE;
    style_pr.image.intense = LV_OPA_100;
    style_pr.text.color = LV_COLOR_HEX3(0xaaa);

    lv_obj_t *menuBtn = lv_imgbtn_create(main_cont, NULL);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_REL, &img_menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_PR, &img_menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_TGL_REL, &img_menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_TGL_PR, &img_menu);
    lv_imgbtn_set_style(menuBtn, LV_BTN_STATE_PR, &style_pr);
    lv_imgbtn_set_style(menuBtn, LV_BTN_STATE_TGL_PR, &style_pr);
    lv_imgbtn_set_toggle(menuBtn, true);
    lv_imgbtn_set_action(menuBtn, LV_BTN_ACTION_PR, menubtn_action);
    lv_obj_align(menuBtn, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -30);

    batt_monitor_task();

}


/**********************
 *   STATIC FUNCTIONS
 **********************/
static lv_res_t lv_tileview_action(lv_obj_t *obj, lv_coord_t x, lv_coord_t y)
{
    printf("x:%d y:%d \n", x, y);
    if (x == prev) {
        g_menu_in = false;
        lv_obj_set_hidden(obj, true);
        menu_data[x].callback(g_menu_win);
        curr_index = x;
    } else {
        prev = x;
    }
    return LV_RES_OK;
}


static lv_res_t menubtn_action(lv_obj_t *btn)
{
    create_menu(main_cont);
    return LV_RES_OK;
}

static lv_res_t lv_setting_backlight_action(lv_obj_t *obj, const char *txt)
{
    printf("lv_setting_backlight_action : %s\n", txt);
#ifdef ESP32
    backlight_setting(atoi(txt));
#endif
    return LV_RES_OK;
}


static lv_res_t lv_setting_th_action(lv_obj_t *obj)
{
    char buf[64];
    lv_ddlist_get_selected_str(obj, buf);
    printf("New option selected on a drop down list: %s\n", buf);

    const char *theme_options[] = {
        "Alien", "Night", "Mono", "Nemo", "Material"
    };

    int option = -1;

    for (int i = 0; i < sizeof(theme_options) / sizeof(theme_options[0]); ++i) {
        if (!strcmp(buf, theme_options[i])) {
            option = i;
            break;
        }
    }
    switch (option) {
    case 0:
#if USE_LV_THEME_ALIEN
        lv_theme_set_current(lv_theme_alien_init(100, NULL));
#endif
        break;
    case 1:
#if USE_LV_THEME_NIGHT
        lv_theme_set_current(lv_theme_night_init(100, NULL));
#endif
        break;
    case 2:
#if USE_LV_THEME_ZEN
        lv_theme_set_current(lv_theme_zen_init(100, NULL));
#endif
        break;
    case 3:
#if USE_LV_THEME_MONO
        lv_theme_set_current(lv_theme_mono_init(100, NULL));
#endif
        break;
    case 4:
#if USE_LV_THEME_NEMO
        lv_theme_set_current(lv_theme_nemo_init(100, NULL));
#endif
        break;
    case 5:
#if USE_LV_THEME_MATERIAL
        lv_theme_set_current(lv_theme_material_init(100, NULL));
#endif
        break;
    default:
        break;
    }
    return LV_RES_OK;
}





//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void lv_menu_1()
{
    static const lv_point_t vp[] = {
        // {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0},
        {0, 0},
        {0, 1}, {1, 1},
        {0, 2}, {1, 2},
        // {0, 3}, {1, 3},
        // {0, 4}, {1, 4},
        {LV_COORD_MIN, LV_COORD_MIN}
    };
    lv_obj_t *t = lv_tileview_create(lv_scr_act(), NULL);
    lv_tileview_set_valid_positions(t, vp);
    lv_obj_set_size(t, LV_HOR_RES, LV_VER_RES);

    lv_obj_t *label;

    lv_obj_t *p00 = lv_obj_create(t, NULL);
    lv_obj_set_click(p00, true);
    lv_obj_set_style(p00, &lv_style_pretty_color);
    lv_obj_set_size(p00, lv_obj_get_width(t), lv_obj_get_height(t));
    lv_tileview_add_element(p00);
    label = lv_label_create(p00, NULL);
    lv_label_set_text(label, "x0, y0");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *p01 = lv_obj_create(t, p00);
    lv_obj_align(p01, p00, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_tileview_add_element(p01);
    label = lv_label_create(p01, NULL);
    lv_label_set_text(label, "x0, y1");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *p11 = lv_obj_create(t, p00);
    lv_obj_align(p11, p01, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_tileview_add_element(p11);
    label = lv_label_create(p11, NULL);
    lv_label_set_text(label, "x1, y1");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);


    lv_obj_t *p02 = lv_obj_create(t, p00);
    lv_obj_align(p02, p01, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_tileview_add_element(p02);
    label = lv_label_create(p02, NULL);
    lv_label_set_text(label, "x0, y2");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *p12 = lv_obj_create(t, p00);
    lv_obj_align(p12, p02, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_tileview_add_element(p12);
    label = lv_label_create(p12, NULL);
    lv_label_set_text(label, "x1, y2");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
}
