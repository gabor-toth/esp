#ifndef GPIO_TASK_H
#define GPIO_TASK_H

#include <stdint.h>

typedef void (*gpio_change_callback)( uint32_t io_num, int state );

extern void gpio_task_init( gpio_change_callback callback );

extern void gpio_task_add( int io_num );

#endif //GPIO_TASK_H
