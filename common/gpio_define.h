#ifndef ONTOZO_GPIO_DEFINE_H
#define ONTOZO_GPIO_DEFINE_H

#include <stdint.h>
#include <stdbool.h>
#include <hal/gpio_types.h>
#include <driver/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PIN_ENABLED     1
#define PIN_DISABLED    0

// types

#define INPUTS   true
#define OUTPUTS  false

typedef struct {
    char *name;
    bool is_manual;
    bool state;
} PinData;

typedef enum {
    high_is_on, low_is_on, inherit = -1
} PinLevelType;

typedef void (*gpio_changed_callback_t)( gpio_num_t io_num, int state );

extern void gpio_init( void *user_context, gpio_changed_callback_t gpio_changed_callback );

extern void gpio_add_class( bool is_input, const char *name, int max_pin_count, PinLevelType level_type );

extern int
gpio_add_pin( bool is_input, int class_id, gpio_num_t gpio_pin, PinLevelType level_type, uint64_t *pin_bit_mask );

extern void gpio_set_delays( bool is_input, int class_id, int index, int delay_ms_going_low, int delay_ms_going_high );

extern void gpio_define_output_pins_callback( gpio_config_t *io_conf, void *user_context );

extern void gpio_define_input_pins_callback( gpio_config_t *io_conf, void *user_context );

extern int gpio_get_number_of_classes( bool is_input );

extern const char *gpio_get_class_name( bool is_input, int class_id );

extern int gpio_get_number_of_pins( bool is_input, int class_id );

extern bool gpio_is_valid_index( bool is_input, int class_id, int index );

extern bool gpio_get_pin_state( bool is_input, int class_id, int index );

extern bool gpio_get_pin_data( bool is_input, int class_id, int index, PinData *pin_data );

extern void gpio_set_pin_state( bool is_input, int class_id, int index, bool state );

extern void gpio_set_pin_state_forced( bool is_input, int class_id, int index, bool state );

extern bool gpio_set_pin_data( bool is_input, int class_id, int index, PinData *pin_data );

#ifdef __cplusplus
}
#endif

#endif //ONTOZO_GPIO_DEFINE_H
