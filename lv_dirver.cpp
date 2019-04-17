#include "lv_dirver.h"
#include <TFT_eSPI.h>
#include <Ticker.h>
#include "board_def.h"
#include <lvgl.h>
// #include "lv_setting.h"
#include "freertos/FreeRTOS.h"
#include <FT5206.h>

#define BACKLIGHT_CHANNEL   ((uint8_t)1)

static TFT_eSPI *tft = nullptr;
static FT5206_Class *tp = nullptr;
static Ticker lvTicker1;
static Ticker lvTicker2;
static bool tpInit = false;
static TimerHandle_t touch_handle = NULL;



void touch_timer_callback( TimerHandle_t xTimer )
{
#ifdef DEBUG_DEMO
    if (touch_handle) {
        xTimerDelete(touch_handle, portMAX_DELAY);
        touch_handle = NULL;
        backlight_off();
    }
#endif
}

void touch_timer_create()
{
#ifdef DEBUG_DEMO
    if (!touch_handle) {
        touch_handle =  xTimerCreate("tp", 10000 / portTICK_PERIOD_MS, pdTRUE, (void *)0, touch_timer_callback);
        xTimerStart(touch_handle, portMAX_DELAY);
    }
#endif
}

static void touch_timer_reset()
{
#ifdef DEBUG_DEMO
    if (touch_handle)
        xTimerReset(touch_handle, portMAX_DELAY);
#endif
}

static void ex_disp_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_array)
{
#if 1
    uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1) * 2;
    tft->setAddrWindow(x1, y1, x2, y2);
    tft->pushColors((uint8_t *)color_array, size);
#else
    uint16_t c;
    tft->startWrite(); /* Start new TFT transaction */
    tft->setAddrWindow(x1, y1, (x2 - x1 + 1), (y2 - y1 + 1)); /* set the working window */
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            c = color_array->full;
            tft->writeColor(c, 1);
            color_array++;
        }
    }
    tft->endWrite(); /* terminate TFT transaction */
#endif
    lv_flush_ready();
}

int tftGetScreenWidth()
{
    return tft->width();
}

int tftGetScreenHeight()
{
    return tft->height();
}

void display_off()
{
    tft->writecommand(TFT_DISPOFF);
    tft->writecommand(TFT_SLPIN);
    tp->enterSleepMode();
}

void display_sleep()
{
    tft->writecommand(TFT_DISPOFF);
    tft->writecommand(TFT_SLPIN);
    tp->enterMonitorMode();
}

void display_wakeup()
{
    tft->writecommand(TFT_SLPOUT);
    tft->writecommand(TFT_DISPON);
}

void display_init()
{
    tft = new TFT_eSPI(LV_HOR_RES, LV_VER_RES);
    tft->init();
    tft->setRotation(0);

    pinMode(TP_INT, INPUT);
    Wire.begin(I2C_SDA, I2C_SCL);
    // Wire.setClock(200000);
    tp = new FT5206_Class();
    if (! tp->begin(Wire)) {  // pass in 'sensitivity' coefficient
        Serial.println("Couldn't start FT6206 touchscreen controller");
    } else {
        tpInit = true;
        Serial.println("Capacitive touchscreen started");
    }

    lv_init();

    /*Initialize the display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.disp_flush = ex_disp_flush; /*Used in buffered mode (LV_VDB_SIZE != 0  in lv_conf.h)*/
    lv_disp_drv_register(&disp_drv);

    /*Initialize the touch pad*/
    lv_indev_drv_t indev_drv;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read =  [] (lv_indev_data_t *data) -> bool {
        static TP_Point p;
        if (!tpInit)    return false;
        data->state = tp->touched() ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
        if (data->state == LV_INDEV_STATE_PR)
        {
            p = tp->getPoint();
            p.x = map(p.x, 0, 320, 0, 240);
            p.y = map(p.y, 0, 320, 0, 240);
            touch_timer_reset();
            // Serial.printf("x:%d y:%d\n", p.x, p.y);
        }
        /*Set the coordinates (if released use the last pressed coordinates)*/
        data->point.x = p.x;
        data->point.y = p.y;
        return false; /*Return false because no moare to be read*/
    };
    lv_indev_drv_register(&indev_drv);

    lvTicker1.attach_ms(1, [] {
        lv_tick_inc(1);
    });

    lvTicker2.attach_ms(5, [] {
        lv_task_handler();
    });

    touch_timer_create();
}



void backlight_init(void)
{
    ledcAttachPin(TFT_BL, 1); // assign RGB led pins to channels
    ledcSetup(BACKLIGHT_CHANNEL, 12000, 8);   // 12 kHz PWM, 8-bit resolution
    // ledcWrite(BACKLIGHT_CHANNEL, 255);
}


uint8_t backlight_getLevel()
{
    return ledcRead(BACKLIGHT_CHANNEL);
}

void backlight_adjust(uint8_t level)
{
    ledcWrite(BACKLIGHT_CHANNEL, level);
}

void backlight_setting(unsigned char level)
{
    switch (level) {
    case 1:
        ledcWrite(BACKLIGHT_CHANNEL, 100);
        break;
    case 2:
        ledcWrite(BACKLIGHT_CHANNEL, 200);
        break;
    case 3:
        ledcWrite(BACKLIGHT_CHANNEL, 255);
        break;
    default:
        break;
    }
}

bool isBacklightOn()
{
    return (bool)ledcRead(BACKLIGHT_CHANNEL);
}

void backlight_off()
{
    ledcWrite(BACKLIGHT_CHANNEL, 0);
}

void backlight_on()
{
    ledcWrite(BACKLIGHT_CHANNEL, 200);
}

