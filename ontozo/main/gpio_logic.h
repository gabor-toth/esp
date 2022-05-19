#ifndef ONTOZO_GPIO_LOGIC_H
#define ONTOZO_GPIO_LOGIC_H

extern void gpio_init();

// levels

extern int gpio_get_level_state( int level );

extern int gpio_get_number_of_levels();

// zones

extern int gpio_get_number_of_zones();

extern int gpio_is_zone_valid( int zone );

extern int gpio_get_zone_state( int zone );

extern char *gpio_get_zone_name( int zone );

extern void gpio_set_zone_state( int zone, int state );

extern void gpio_set_zone_name( int zone, char *name );

// pumps

extern int gpio_get_number_of_pumps();

extern int gpio_get_pump_state( int pump );

extern char *gpio_get_pump_name( int pump );

extern void gpio_set_pump_state( int pump, int state );

extern void gpio_set_pump_name( int pump, char *name );

#endif //ONTOZO_GPIO_LOGIC_H
