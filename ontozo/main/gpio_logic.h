#ifndef ONTOZO_GPIO_LOGIC_H
#define ONTOZO_GPIO_LOGIC_H

extern void gpio_init();

// inputs

extern int gpio_get_level_state( int level );

extern int gpio_get_number_of_levels();

// outputs

#define PUMPS 0
#define ZONES 1

extern int gpio_get_number_of_output_classes();

extern char *gpio_get_class_name( int class );

extern int gpio_get_number_of_output_pins( int class );

extern bool gpio_is_valid_output_index( int class, int index );

extern bool gpio_get_output_pin_state( int class, int index );

extern char *gpio_get_output_pin_name( int class, int index );

extern void gpio_set_output_pin_state( int class, int index, bool state );

extern void gpio_set_output_pin_name( int class, int index, char *name );

#endif //ONTOZO_GPIO_LOGIC_H
