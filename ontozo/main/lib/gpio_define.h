#ifndef ONTOZO_GPIO_DEFINE_H
#define ONTOZO_GPIO_DEFINE_H

#include <stdint.h>
#include <stdbool.h>
#include <hal/gpio_types.h>

extern void gpio_init();

#define PIN_ENABLED     1
#define PIN_DISABLED    0

// types

#define INPUTS   1
#define OUTPUTS  0

typedef enum {
    high_is_on, low_is_on, inherit = -1
} PinLevelType;

extern void gpio_add_class( bool is_input, char *name, int max_pin_count, PinLevelType level_type );

extern void
gpio_add_pin( bool is_input, int class, gpio_num_t pin, char *name, PinLevelType level_type, uint64_t *pin_bit_mask );

extern void
gpio_add_pin_with_allocated_name( bool is_input, int class, gpio_num_t pin, char *name, PinLevelType level_type,
                                  uint64_t *pin_bit_mask );

extern void gpio_define_output_pins_callback( gpio_config_t *io_conf );

extern void gpio_define_input_pins_callback( gpio_config_t *io_conf );

extern void gpio_changed_callback( uint32_t io_num, int state );

extern int gpio_get_number_of_classes( bool is_input );

extern char *gpio_get_class_name( bool is_input, int class );

extern int gpio_get_number_of_pins( bool is_input, int class );

extern bool gpio_is_valid_index( bool is_input, int class, int index );

extern bool gpio_get_pin_state( bool is_input, int class, int index );

extern char *gpio_get_pin_name( bool is_input, int class, int index );

extern void gpio_set_pin_state( bool is_input, int class, int index, bool state );

extern void gpio_set_pin_name( bool is_input, int class, int index, char *name );

#endif //ONTOZO_GPIO_DEFINE_H
