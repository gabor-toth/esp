#ifndef GPIO_TASK_H
#define GPIO_TASK_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

typedef struct {
    uint32_t io_num;
    TimerHandle_t timer;
    int last_reported_state;
} GpioTimer;

typedef void (*gpio_change_callback)( uint32_t io_num, int state );

extern void gpio_task_init( gpio_change_callback callback );

#endif //GPIO_TASK_H
