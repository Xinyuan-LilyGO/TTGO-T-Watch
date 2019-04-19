#include "lv_play.h"
#include "struct_def.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"

#ifdef LV_PLAY_AUDIO

#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourceSD.h"
// #include "AudioFileSourceSPIFFS.h"
// #include "AudioOutputI2SNoDAC.h"

// AudioFileSourceSPIFFS *file = nullptr;
// AudioOutputI2SNoDAC *out = = nullptr;
static AudioFileSourceID3 *id3 = nullptr;
static AudioGeneratorMP3 *mp3 = nullptr;
static AudioFileSourceSD *file = nullptr;
static AudioOutputI2S *out = nullptr;

static EventGroupHandle_t play_event_group = NULL;

#define PLAY_TASK_BIT   0x01

void play_task(void *param)
{
    for (;;) {
        EventBits_t bits =  xEventGroupWaitBits(play_event_group,
                                                PLAY_TASK_BIT,
                                                pdFALSE, pdFALSE, portMAX_DELAY);
        if (bits & PLAY_TASK_BIT) {
            if (mp3->isRunning()) {
                if (!mp3->loop()) {
                    mp3->stop();
                    Serial.println("stop Done..");
                    xEventGroupClearBits(play_event_group,  PLAY_TASK_BIT );
                }
            } else {
                Serial.println("Done..");
            }
        }
    }
}

void play_handle(void *param)
{
    static bool start = false;
    play_struct_t *p = (play_struct_t *)param;
    switch (p->event) {
    case LVGL_PLAY_START:
        if (!start) {
            start = true;
            Serial.printf("LVGL_PLAY_START: %s\n", p->name);
            file = new AudioFileSourceSD(p->name);
            id3 = new AudioFileSourceID3(file);
            out = new AudioOutputI2S(0, 1);
            mp3 = new AudioGeneratorMP3();
            mp3->begin(id3, out);
            xEventGroupSetBits(play_event_group, PLAY_TASK_BIT);
        }
        break;

    case LVGL_PLAY_STOP:
        if (start) {
            start = false;
            Serial.printf("%s\n", p->name);
            if (mp3->isRunning()) {
                mp3->stop();
            }
            delete file;
            delete id3;
            delete out;
            delete mp3;
            xEventGroupClearBits(play_event_group,  PLAY_TASK_BIT);
        }
        break;
    case LVGL_PLAY_PREV:
    case LVGL_PLAY_NEXT:
        if (start) {
            xEventGroupClearBits(play_event_group,  PLAY_TASK_BIT);
            Serial.printf("LVGL_PLAY_PREV: %s\n", p->name);
            if (mp3->isRunning()) {
                mp3->stop();
            }
            delete file;
            delete id3;
            file = new AudioFileSourceSD(p->name);
            id3 = new AudioFileSourceID3(file);
            mp3->begin(id3, out);
            xEventGroupSetBits(play_event_group, PLAY_TASK_BIT);
        }
        break;
    default:
        break;
    }
}

#endif



void plat_task_init()
{
#ifdef LV_PLAY_AUDIO
    play_event_group = xEventGroupCreate();
    xTaskCreate(play_task, "play", 2048, NULL, 20, NULL);
#endif
}