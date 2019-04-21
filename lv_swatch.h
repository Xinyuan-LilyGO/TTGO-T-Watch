/**
 * @file lv_test_tileview.h
 *
 */

#ifndef LV_SWATCH_H
#define LV_SWATCH_H

#ifdef __cplusplus
extern "C" {
#endif
#include "struct_def.h"
/*********************
 *      INCLUDES
 *********************/


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a tileview to test their functionalities
 */
void lv_main(void);
void lv_main_time_update(const char *time, const char *date);
void lv_main_step_counter_update(const char *step);
bool lv_main_in();

void gps_anim_close();
void gps_create_static_text();
void motion_dir_update(uint8_t index);
void lv_file_list_add(const char *filename,uint8_t type);

uint8_t lv_gps_static_text_update(void *data);
uint8_t lv_wifi_list_add(const char *ssid,int32_t rssi, uint8_t ch);
void lv_wifi_connect_pass();
void lv_wifi_connect_fail();
void lv_update_power_info(power_data_t *data);

void charging_anim_start();
void charging_anim_stop();
void lv_create_ttgo();
void lora_add_message(const char *txt);
void lv_update_battery_percent(int percent);
void lv_music_list_add(const char *name);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_TEST_TILEVIEW_H*/
