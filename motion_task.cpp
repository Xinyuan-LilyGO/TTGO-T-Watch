#include "motion_task.h"
#include <bma423.h>
#include <Arduino.h>
#include <Wire.h>
#include "board_def.h"
#include "FreeRTOS.h"
#include "freertos/queue.h"
#include "struct_def.h"
#include "lv_swatch.h"

#define BMA423_FEATURE_SIZE 64
#define ACCEL_ENABLE    1
#define ACCEL_DISABLE   0

extern xQueueHandle g_event_queue_handle;
static struct bma4_dev bmd4_dev;
static EventGroupHandle_t motionEventGroup = NULL;
static motion_struct_t motion_data;
static uint32_t step_counter = 0;

static int scanI2Cdevice(void);
static uint16_t _bma423_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *read_data, uint16_t len);
static uint16_t _bma423_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *write_data, uint16_t len);
static uint16_t bma423_accel_enable();
static uint16_t bma_get_dir(direction_t *result);
static uint16_t configure_interrupt();
static uint16_t bma423_read_temp(float *temp, uint8_t unit = BMA4_DEG);
static void bma423_soft_reset();

#define MOTION_GET_DATA_BIT 0X01
#define MOTION_GET_TEMP_BIT 0X02
#define MOTION_GET_SETP_BIT 0X04

static void motion_task(void *param)
{
    float temp;
    struct bma4_accel sens_data;
    char buf[64];
    for (;;) {
        EventBits_t bits =  xEventGroupWaitBits(motionEventGroup,
                                                MOTION_GET_DATA_BIT | MOTION_GET_TEMP_BIT | MOTION_GET_SETP_BIT,
                                                pdFALSE, pdFALSE, portMAX_DELAY);
        if (bits & MOTION_GET_DATA_BIT) {
            direction_t result;
            if ( bma_get_dir(&result) == BMA4_OK) {
                motion_dir_update((uint8_t)result);
            }
        } else if (bits & MOTION_GET_TEMP_BIT) {
            bma423_read_temp(&temp);
            snprintf(buf, sizeof(buf), "%.2f", temp);
            // Serial.println(buf);
            lv_main_temp_update(buf);
            xEventGroupClearBits(motionEventGroup,  MOTION_GET_TEMP_BIT );

        } else if (bits & MOTION_GET_SETP_BIT) {
            uint16_t int_status = 0;
            uint32_t stepCount;
            uint16_t  rlst;
            // bma423_read_int_status(&int_status, &bmd4_dev);
            // Serial.printf("Read MOTION_GET_SETP_BIT[0x%x]\n", int_status);
            do {
                rlst = bma423_read_int_status(&int_status, &bmd4_dev);
            } while (rlst != BMA4_OK);

            if (int_status & BMA423_STEP_CNTR_INT) {
                if (bma423_step_counter_output(&stepCount, &bmd4_dev) == BMA4_OK) {
                    snprintf(buf, sizeof(buf), "%u", stepCount);
                    Serial.println(stepCount);
                    lv_main_step_counter_update(buf);
                }
            } else if (int_status & BMA423_WAKEUP_INT) {
                Serial.printf("BMA423_WAKEUP_INT\n");
            }
            xEventGroupClearBits(motionEventGroup,  MOTION_GET_SETP_BIT );
        }
    }
}

void motion_handle(void *arg)
{
    motion_struct_t *p = (motion_struct_t *)arg;
    switch ((p->event)) {
    case LVGL_MOTION_STOP:
        Serial.println("[BMA] bma423 power off ...");
        // bma4_set_accel_enable(ACCEL_DISABLE, &bmd4_dev);
        xEventGroupClearBits(motionEventGroup, MOTION_GET_DATA_BIT | MOTION_GET_TEMP_BIT | MOTION_GET_SETP_BIT);
        break;

    case LVGL_MOTION_GET_ACCE:
        // Serial.println("[BMA] bma423 get accel ...");
        // bma423_accel_enable();
        xEventGroupSetBits(motionEventGroup, MOTION_GET_DATA_BIT);
        break;

    case LVGL_MOTION_GET_STEP:
        // Serial.println("[BMA] bma423 get step ...");
        xEventGroupSetBits(motionEventGroup, MOTION_GET_SETP_BIT);
        break;

    case LVGL_MOTION_GET_TEMP:
        xEventGroupSetBits(motionEventGroup, MOTION_GET_TEMP_BIT);
        break;
    default:
        break;
    }
}

bool motion_task_init()
{
    uint16_t rslt = BMA4_OK;

    //! configure i2c  触摸已经初始化了，i2c多线程调用将产生冲突
    // Wire1.begin(SEN_SDA, SEN_SCL);
    if (!scanI2Cdevice()) {
        return false;
    }
    /* Modify the parameters */
    bmd4_dev.dev_addr        = BMA4_I2C_ADDR_SECONDARY;
    bmd4_dev.interface       = BMA4_I2C_INTERFACE;
    bmd4_dev.bus_read        = _bma423_read;
    bmd4_dev.bus_write       = _bma423_write;
    bmd4_dev.delay           = delay;
    bmd4_dev.read_write_len  = 8;
    bmd4_dev.resolution      = 12;
    bmd4_dev.feature_len     = BMA423_FEATURE_SIZE;

    bma423_soft_reset();

    // ! 1. Sensor API initialization for the I2C protocol
    /* a. Reading the chip id. */
    rslt = bma423_init(&bmd4_dev);
    if (rslt != BMA4_OK) {
        Serial.println("bma4 init fail");
        return false;
    }
    /* b. Performing initialization sequence.
        c. Checking the correct status of the initialization sequence.
    */
    //! 2. Write configure file
    rslt = bma423_write_config_file(&bmd4_dev);
    if (rslt != BMA4_OK) {
        Serial.println("bma4 write config fail");
        return false;
    }
    Serial.println("Write config file complete ... ");

    configure_interrupt();

    motionEventGroup = xEventGroupCreate();

    xTaskCreatePinnedToCore(motion_task, "motion", 4096, NULL, 20, NULL, 0);

    return true;
}

static void bma423_soft_reset()
{
    uint8_t reg = 0xB6;
    _bma423_write(BMA4_I2C_ADDR_SECONDARY, 0x7E, &reg, 1);
    delay(5);
}

#define ENABLE_BMA_INT2
static uint16_t configure_interrupt()
{
    uint16_t rslt = BMA4_OK;
    rslt |= bma423_accel_enable();
    rslt |= bma423_step_counter_set_watermark(100, &bmd4_dev);
    rslt |= bma423_reset_step_counter(&bmd4_dev);
    rslt |= bma423_feature_enable(BMA423_STEP_CNTR, BMA4_ENABLE, &bmd4_dev);
    rslt |= bma423_feature_enable(BMA423_WAKEUP, BMA4_ENABLE, &bmd4_dev);
    rslt |= bma423_step_detector_enable(BMA4_ENABLE, &bmd4_dev);
#ifdef ENABLE_BMA_INT1
    rslt |= bma423_map_interrupt(BMA4_INTR1_MAP, BMA423_STEP_CNTR_INT | BMA423_WAKEUP_INT, BMA4_ENABLE, &bmd4_dev);
#endif

#ifdef ENABLE_BMA_INT2
    rslt |= bma423_map_interrupt(BMA4_INTR2_MAP, BMA423_STEP_CNTR_INT | BMA423_WAKEUP_INT, BMA4_ENABLE, &bmd4_dev);
#endif

    Serial.printf("configure_interrupt rslt : %x\n", rslt);

    struct bma4_int_pin_config config ;
    config.edge_ctrl = BMA4_LEVEL_TRIGGER;
    config.lvl = BMA4_ACTIVE_LOW;
    config.od = BMA4_PUSH_PULL;
    config.output_en = BMA4_OUTPUT_ENABLE;
    config.input_en = BMA4_INPUT_DISABLE;

#ifdef ENABLE_BMA_INT1
    rslt |= bma4_set_int_pin_config(&config, BMA4_INTR1_MAP, &bmd4_dev);
#endif

#ifdef ENABLE_BMA_INT2
    rslt |= bma4_set_int_pin_config(&config, BMA4_INTR2_MAP, &bmd4_dev);
#endif

    Serial.printf("configure_interrupt rslt : %x\n", rslt);

#ifdef ENABLE_BMA_INT1
    pinMode(BMA423_INT1, INPUT_PULLUP);
    attachInterrupt(BMA423_INT1, [] {
        // Serial.println("attachInterrupt");
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        task_event_data_t event_data;
        event_data.type = MESS_EVENT_MOTI;
        event_data.motion.event = LVGL_MOTION_GET_STEP;
        xQueueSendFromISR(g_event_queue_handle, &event_data, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken)
        {
            portYIELD_FROM_ISR ();
        }
    }, FALLING);
#endif

#ifdef ENABLE_BMA_INT2
    pinMode(BMA423_INT2, INPUT_PULLUP);
    attachInterrupt(BMA423_INT2, [] {
        // Serial.println("attachInterrupt");
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        task_event_data_t event_data;
        event_data.type = MESS_EVENT_MOTI;
        event_data.motion.event = LVGL_MOTION_GET_STEP;
        xQueueSendFromISR(g_event_queue_handle, &event_data, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken)
        {
            portYIELD_FROM_ISR ();
        }
    }, FALLING);
#endif
}


static uint16_t bma_get_dir(direction_t *result)
{
    uint16_t rslt = BMA4_OK;
    struct bma4_accel accel;
    uint16_t absX, absY, absZ;
    rslt = bma4_read_accel_xyz(&accel, &bmd4_dev);
    if (rslt != BMA4_OK) {
        Serial.println("Read fail");
        return rslt;
    }
    absX = abs(accel.x);
    absY = abs(accel.y);
    absZ = abs(accel.z);
    if ((absZ > absX) && (absZ > absY)) {
        if (accel.z > 0) {
            *result = DIRECTION_DISP_DOWN;
        } else {
            *result = DIRECTION_DISP_UP;
        }
    } else if ((absY > absX) && (absY > absZ)) {
        if (accel.y > 0) {
            *result = DIRECTION_BOTTOM_EDGE;
        } else {
            *result = DIRECTION_TOP_EDGE;
        }
    } else {
        if (accel.x < 0) {
            *result = DIRECTION_RIGHT_EDGE;
        } else {
            *result = DIRECTION_LEFT_EDGE;
        }
    }
    return rslt;
}


static uint16_t bma423_read_temp(float *temp, uint8_t unit)
{
    uint16_t rslt;
    int32_t get_temp = 0;
    if (unit > 0 && unit < 4) {
        rslt = bma4_get_temperature(&get_temp, unit, &bmd4_dev);
        *temp = (float)get_temp / (float)BMA4_SCALE_TEMP;
        /* 0x80 - temp read from the register and 23 is the ambient temp added.
         * If the temp read from register is 0x80, it means no valid
         * information is available */
        if (unit == BMA4_DEG) {
            if (((get_temp - 23) / BMA4_SCALE_TEMP) == 0x80) {
                rslt = BMA4_E_FAIL;
            }
        }
    } else {
        rslt = BMA4_E_FAIL;
    }
    return rslt;
}


static uint16_t bma423_accel_enable()
{
    uint16_t rslt = BMA4_OK;
    //! 3. Configuring the accelerometer
    /* Enable the accelerometer */
    rslt = bma4_set_accel_enable(ACCEL_ENABLE, &bmd4_dev);
    if (rslt != BMA4_OK) {
        Serial.println("bma4 enable accel fail");
        return rslt;
    }
    /* Declare an accelerometer configuration structure */
    struct bma4_accel_config accel_conf;

    /* Assign the desired settings */
    accel_conf.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    accel_conf.range = BMA4_ACCEL_RANGE_2G;
    accel_conf.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    accel_conf.perf_mode = BMA4_CONTINUOUS_MODE;

    /* Set the configuration */
    rslt = bma4_set_accel_config(&accel_conf, &bmd4_dev);
    if (rslt != BMA4_OK) {
        Serial.println("bma4 set accel config fail");
        return rslt;
    }
    return rslt;
}

static int scanI2Cdevice(void)
{
    byte err, addr;
    int nDevices = 0;
    for (addr = 1; addr < 127; addr++) {
        Wire1.beginTransmission(addr);
        err = Wire1.endTransmission();
        if (err == 0) {
            Serial.print("I2C device found at address 0x");
            if (addr < 16)
                Serial.print("0");
            Serial.print(addr, HEX);
            Serial.println(" !");
            nDevices++;
        } else if (err == 4) {
            Serial.print("Unknow error at address 0x");
            if (addr < 16)
                Serial.print("0");
            Serial.println(addr, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("done\n");
    return nDevices;
}

/*!
 * @brief Bus communication function pointer which should be mapped to
 * the platform specific read and write functions of the user
 */
static uint16_t _bma423_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *read_data, uint16_t len)
{
    uint16_t ret = 0;
    Wire1.beginTransmission(dev_addr);
    Wire1.write(reg_addr);
    Wire1.endTransmission(false);
    uint8_t cnt = Wire1.requestFrom(dev_addr, len, true);
    if (!cnt) {
        ret =  1 << 13;
    }
    uint16_t index = 0;
    while (Wire1.available()) {
        read_data[index++] = Wire1.read();
    }
    return ret;
}

static uint16_t _bma423_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *write_data, uint16_t len)
{
    uint16_t ret = 0;
    char *err = NULL;
    Wire1.beginTransmission(dev_addr);
    Wire1.write(reg_addr);
    for (uint16_t i = 0; i < len; i++) {
        Wire1.write(write_data[i]);
    }
    ret =  Wire1.endTransmission();
    return ret ? 1 << 12 : ret;
}