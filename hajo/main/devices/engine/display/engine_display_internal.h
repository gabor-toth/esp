#ifndef HAJO_ENGINE_DISPLAY_INTERNAL_H
#define HAJO_ENGINE_DISPLAY_INTERNAL_H

#include <cstdint>

#define CONFIG_ENGINE_DISPLAY_IDLE_TIMEOUT_SECS     5
#define CONFIG_ENGINE_DISPLAY_LOGO_SECS             3
#define CONFIG_ENGINE_DISPLAY_OFF_TIMEOUT_SECS      15 // 120
#define CONFIG_ENGINE_DISPLAY_SHOW_LEADING_ZEROES   false

#define PIN_LCD_CLK     GPIO_NUM_7      // E
#define PIN_LCD_MOSI    GPIO_NUM_5      // RW
#define PIN_LCD_CS      GPIO_NUM_3      // RS
#define PIN_LCD_RESET   GPIO_NUM_9      // RST
#define PIN_LCD_BACKLIGHT       GPIO_NUM_11
#define PIN_BUTTON_BACKLIGHT    GPIO_NUM_12

#define PIN_LCD2_SI     GPIO_NUM_6
#define PIN_LCD2_SCL    GPIO_NUM_8
#define PIN_LCD2_A0     GPIO_NUM_10
#define PIN_LCD2_RST    GPIO_NUM_13
#define PIN_LCD2_CS1B   GPIO_NUM_14

#define PIN_INPUT_ONOFF     GPIO_NUM_39
#define PIN_INPUT_START     GPIO_NUM_37
#define PIN_INPUT_STOP      GPIO_NUM_35
#define PIN_INPUT_LIGHT     GPIO_NUM_33

// GND
// VCC = 5V
// VO = 10k pot middle pin
// BLA = LED+ Backlight 5V
// BLK = LED– Backlight GND
// PSB = LOW -> SPI mode

typedef struct {
    int16_t rpm;
    int16_t hours;
    bool chargerFailure;
    bool oilPressureFailure;
    bool coolingWaterTemperatureFailure;
    bool hasFailure;
    bool flashState;
} display_data_t;

extern display_data_t displayData;

#endif //HAJO_ENGINE_DISPLAY_INTERNAL_H
