#include "lv_dirver.h"
#include <TFT_eSPI.h>
#include <Ticker.h>
#include "board_def.h"
#include <Adafruit_FT6206.h>
#include <lvgl.h>
#include "lv_setting.h"


static TFT_eSPI *tft = nullptr;
static Adafruit_FT6206 *tp = nullptr;
static Ticker lvTicker1;
static Ticker lvTicker2;
static bool tpInit = false;

static TimerHandle_t touch_handle = NULL;
void touch_timer_callback( TimerHandle_t xTimer )
{
    if (touch_handle) {
        xTimerDelete(touch_handle, portMAX_DELAY);
        touch_handle = NULL;
        backlight_off();
    }
}

void touch_timer_create()
{
    if (!touch_handle) {
        touch_handle =  xTimerCreate("tp", 10000 / portTICK_PERIOD_MS, pdTRUE, (void *)0, touch_timer_callback);
        xTimerStart(touch_handle, portMAX_DELAY);
    }
}

static void touch_timer_reset()
{
    if (touch_handle)
        xTimerReset(touch_handle, portMAX_DELAY);
}

static void ex_disp_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_array)
{
#if 0
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
    tp->sleepIn();
}

void display_init()
{
    tft = new TFT_eSPI(LV_HOR_RES, LV_VER_RES);
    tft->init();
    tft->setRotation(0);

    pinMode(TP_INT, INPUT);
    Wire.begin(I2C_SDA, I2C_SCL);
    // Wire.setClock(200000);
    tp = new Adafruit_FT6206();
    if (! tp->begin(40)) {  // pass in 'sensitivity' coefficient
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
        static TS_Point p;
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




class FT5206_Class
{
#define FT5206_SLAVE_ADDRESS    (0x38)

#define FT5206_MODE_REG         (0x00)
#define FT5206_TOUCHES_REG      (0x02)
#define FT5206_VENDID_REG       (0xA8)
#define FT5206_CHIPID_REG       (0xA3)
#define FT5206_THRESHHOLD_REG   (0x80)
#define FT5206_POWER_REG        (0x87)

#define FT5206_SLEEP_IN        (0x03)

#define FT5206_VENDID           0x11
#define FT6206_CHIPID           0x06
#define FT6236_CHIPID           0x36
#define FT6236U_CHIPID          0x64

public:
    FT5206_Class();

    int begin(TwoWire &port = Wire, uint8_t addr = FT5206_SLAVE_ADDRESS)
    {
        uint8_t val;
        _readByte(FT5206_VENDID_REG, 1, &val);
        if (val != FT5206_VENDID) {
            return false;
        }
        _readByte(FT5206_CHIPID_REG, 1, &val);
        if ((val != FT6206_CHIPID) && (val != FT6236_CHIPID) && (val != FT6236U_CHIPID)) {
            return false;
        }
    }

    // valid touching detect threshold.
    void adjustTheshold(uint8_t thresh)
    {
        _writeByte(FT5206_THRESHHOLD_REG, 1, &thresh);
    }

    bool getPoint()
    {
        _readByte(0x00, 16, _data);
        // touches = _data[0x02];
        // if ((touches > 2) || (touches == 0)) {
        //     touches = 0;
        // }
        for (uint8_t i = 0; i < 2; i++) {
            _x[i] = _data[0x03 + i * 6] & 0x0F;
            _x[i] <<= 8;
            _x[i] |= _data[0x04 + i * 6];
            _y[i] = _data[0x05 + i * 6] & 0x0F;
            _y[i] <<= 8;
            _y[i] |= _data[0x06 + i * 6];
            _id[i] = _data[0x05 + i * 6] >> 4;
        }
    }

    uint8_t isTouched()
    {
        uint8_t val;
        _readByte(FT5206_TOUCHES_REG, 1, &val);
        return val;
    }

    void sleep()
    {
        uint8_t val = FT5206_SLEEP_IN;
        _writeByte(FT5206_POWER_REG, 1, &val);
    }

private:

    int _readByte(uint8_t reg, uint8_t nbytes, uint8_t *data)
    {
        _i2cPort->beginTransmission(_address);
        _i2cPort->write(reg);
        _i2cPort->endTransmission();
        _i2cPort->requestFrom(_address, nbytes);
        uint8_t index = 0;
        while (_i2cPort->available())
            data[index++] = _i2cPort->read();
    }

    int _writeByte(uint8_t reg, uint8_t nbytes, uint8_t *data)
    {
        _i2cPort->beginTransmission(_address);
        _i2cPort->write(reg);
        for (uint8_t i = 0; i < nbytes; i++) {
            _i2cPort->write(data[i]);
        }
        _i2cPort->endTransmission();
    }

    uint8_t _address;
    uint8_t _data[16];
    uint16_t _x[2];
    uint16_t _y[2];
    uint16_t _id[2];
    bool _init = false;
    TwoWire *_i2cPort;
};

