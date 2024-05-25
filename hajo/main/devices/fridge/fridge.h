#ifndef HAJO_FRIDGE_H
#define HAJO_FRIDGE_H

extern void fridge_main();

extern void fridge_fan_setup();

extern void fridge_fan_timer_handler();

extern void fridge_fan_set_duty_cycle( int index, double duty_cycle );

extern void fridge_temp_setup();

extern void fridge_temp_timer_handler();

#endif //HAJO_FRIDGE_H
