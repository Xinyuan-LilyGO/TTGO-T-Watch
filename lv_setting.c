// #include <Arduino.h>
// #include "board_def.h"

// #define BACKLIGHT_CHANNEL   ((uint8_t)1)

// void backlight_init(void)
// {
//     ledcAttachPin(TFT_BL, 1); // assign RGB led pins to channels
//     ledcSetup(BACKLIGHT_CHANNEL, 12000, 8);   // 12 kHz PWM, 8-bit resolution
//     // ledcWrite(BACKLIGHT_CHANNEL, 255);
// }


// uint8_t backlight_getLevel()
// {
//     return ledcRead(BACKLIGHT_CHANNEL);
// }

// void backlight_adjust(uint8_t level)
// {
//     ledcWrite(BACKLIGHT_CHANNEL, level);
// }

// void backlight_setting(unsigned char level)
// {
//     switch (level) {
//     case 1:
//         ledcWrite(BACKLIGHT_CHANNEL, 100);
//         break;
//     case 2:
//         ledcWrite(BACKLIGHT_CHANNEL, 200);
//         break;
//     case 3:
//         ledcWrite(BACKLIGHT_CHANNEL, 255);
//         break;
//     default:
//         break;
//     }
// }

// bool isBacklightOn()
// {
//     return (bool)ledcRead(BACKLIGHT_CHANNEL);
// }

// void backlight_off()
// {
//     ledcWrite(BACKLIGHT_CHANNEL, 0);
// }

// void backlight_on()
// {
//     ledcWrite(BACKLIGHT_CHANNEL, 200);
// }

