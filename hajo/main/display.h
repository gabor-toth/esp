#ifndef HAJO_DISPLAY_H
#define HAJO_DISPLAY_H

extern void display_main();

typedef enum {
    FUEL,
    VOLTAGE,
    WATER
} display_type_t;

extern void display_set_value( display_type_t type, int instance, int value );

#endif //HAJO_DISPLAY_H
