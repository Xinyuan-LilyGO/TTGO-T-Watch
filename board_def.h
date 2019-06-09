
#ifndef __BOARD_DEF_H
#define __BOARD_DEF_H

//#define UBOX_GPS_MODULE      //Turning on will open the M8/M6 GPS module
// #define ACSIP_S7XG_MODULE    //Turning on will open the S7XG GPS/LORA module
// #define ENABLE_BLE           //Turning on will open the BLE module

#define TFT_MISO            -1  
#define TFT_MOSI            19  
#define TFT_SCLK            18  
#define TFT_CS              5   
#define TFT_DC              27  
#define TFT_RST             -1  
#define TFT_BL              12   

#define SD_CS               13  
#define SD_MISO             2
#define SD_MOSI             15
#define SD_SCLK             14

#define I2C_SDA             23
#define I2C_SCL             32

#define SEN_SDA             21
#define SEN_SCL             22

#define UART_TX              33
#define UART_RX              34

#if defined(UBOX_GPS_MODULE)
#define UART_BANUD_RATE     9600
#else
#define UART_BANUD_RATE     115200
#endif

#define USER_BUTTON         36

#define TP_INT              38
#define RTC_INT             37
#define AXP202_INT          35
#define BMA423_INT1         39
#define BMA423_INT2         0

#endif
