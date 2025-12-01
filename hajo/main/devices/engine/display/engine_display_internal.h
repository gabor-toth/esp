#ifndef HAJO_ENGINE_DISPLAY_INTERNAL_H
#define HAJO_ENGINE_DISPLAY_INTERNAL_H

#include <cstdint>

#define PIN_CLK     GPIO_NUM_7      // E
#define PIN_MOSI    GPIO_NUM_5      // RW
#define PIN_CS      GPIO_NUM_3      // RS
#define PIN_RESET   GPIO_NUM_9      // RST
#define PIN_BACKLIGHT   GPIO_NUM_11

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
