TTGO T-Watch    
=================================

T-Watch is an ESP32-based smart watch hardware designed by Shenzhen Xinyuan Electronics Co., Ltd. Hardware configuration with main core SOC (ESP32), 16MBytes large flash, 8MBytes external SPRAM, integrated multi-channel programmable power management chip ( AXP202), onboard three-axis accelerometer (BMA423), built-in step counter algorithm and other multi-function GSensor, with a variety of different configurations of internal replaceable backplane, such as LORA, GPS, SIM, SD

# PinOut
| Name         | Pins |
| ------------ | ---- |
| TFT CS       | 5    |
| TFT SCLK     | 18   |
| TFT MOSI     | 19   |
| TFT MISO     | -1   |
| TFT DC       | 27   |
| TFT RST      | -1   |
| TFT BL       | 12   |
| SD CS        | 13   |
| SD MOSI      | 15   |
| SD MISO      | 2    |
| SD SCLK      | 14   |
| TOUCH SDA    | 23   |
| TOUCH SCL    | 32   |
| TOUCH INT    | 38   |
| SENSOR SDA   | 21   |
| SENSOR SCL   | 22   |
| GPS TX       | 33   |
| GPS RX       | 34   |
| S7XG GPS RST | -1   |
| BUTTON       | 36   |
| RTC INT      | 37   |
| AXP202 INT   | 35   |
| BMA423 INT   | 39   |


## Note:

- `AXP202X_Library` will have set the DCDC3 output enable to normally open, because DCDC3 will supply power to the ESP32 main chip. If using other methods of control, please be careful not to turn off DCDC3, otherwise it will not be able to burn the program.
  
- The repository depends on other library files, please recursively clone the repository
    ```
    git clone --recursive https://github.com/lewisxhe/T-Watch
    ```

- Then move the library files in the `Submodules` directory to
    ```
    C:\Users\<UserName>\Documents\Arduino\libraries
    ```
- In the Arduino board select `TTGO T-Watch`, the PSRAM is enabled by default, if it is not turned on, please open it.


- [TFT_eSPI](https://github.com/lewisxhe/TFT_eSPI)
- [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus)
- [lvgl](https://github.com/lewisxhe/lvgl)
- [FT5206_Library](https://github.com/lewisxhe/FT5206_Library)
- [AXP202X_Library](https://github.com/lewisxhe/AXP202X_Library)
- [PCF8563_Library](https://github.com/lewisxhe/PCF8563_Library)
- [BMA423_Library](https://github.com/lewisxhe/BMA423_Library)
- [S7xG_Library](https://github.com/lewisxhe/S7xG_Library)


## Low power consumption:
- In standby mode, the display is off, the current minimum power consumption is about 2.5mA, detailed step reference code to close the screen section
  
### The pictures and fonts used in the program are from the network. If there is any infringement, please inform the author.

