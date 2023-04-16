#ifndef LIB_GPIO_TASK_H
#define LIB_GPIO_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*gpio_change_callback)( uint32_t io_num, int state );

extern void gpio_task_init( gpio_change_callback callback );

extern void gpio_task_add( int io_num, int delay_ms_on_going_low, int delay_ms_on_going_high );

#ifdef __cplusplus
}
#endif

#endif //LIB_GPIO_TASK_H
