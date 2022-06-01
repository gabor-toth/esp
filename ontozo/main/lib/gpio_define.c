#include <string.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "lib/gpio_task.h"
#include "gpio_define.h"

#define ESP_INTR_FLAG_DEFAULT 0
#define LOG_TAG "gpio"

#define reached( X ) ((X)!=0)

#define PUMP_MAIN     0
#define PUMP_REFILL   1

#define MAX_PIN_CLASSES 4

typedef struct {
    gpio_num_t pin;
    char *name;
    PinLevelType level_type;
    bool state;
} Pin;

typedef struct {
    char *name;
    int max_pin_count;
    int used_pin_count;
    Pin *pins;
    PinLevelType level_type;
} PinClass;

typedef struct {
    PinClass classes[MAX_PIN_CLASSES];
    int used_classes;
} PinClasses;

static PinClasses pin_definitions[2];

static bool gpio_is_valid_class( bool is_input, int class ) {
    if ( class >= 0 && class < pin_definitions[ is_input ].used_classes ) {
        return true;
    }
    ESP_LOGE( LOG_TAG, "Wrong pin class %d/%d >= %d/%d",
              is_input, class,
              sizeof( pin_definitions ) / sizeof( pin_definitions[ 0 ] ), pin_definitions[ is_input ].used_classes );
    return false;
}

bool gpio_is_valid_index( bool is_input, int class, int index ) {
    if ( !gpio_is_valid_class( is_input, class )) {
        return false;
    }
    int max_pin_count = pin_definitions[ is_input ].classes[ class ].max_pin_count;
    if ( index >= 0
         && index < max_pin_count ) {
        return true;
    }
    ESP_LOGE( LOG_TAG, "Wrong pin index %d/%d/%d >= %d",
              is_input, class, index,
              max_pin_count
    );
    return false;
}

void gpio_add_class( bool is_input, char *name, int max_pin_count, PinLevelType level_type ) {
    PinClasses *pin_classes = &pin_definitions[ is_input ];
    ESP_LOGI( LOG_TAG, "Adding pin class %d/%d \"%s\"", is_input, pin_classes->used_classes, name );
    if ( pin_classes->used_classes == MAX_PIN_CLASSES ) {
        ESP_LOGE( LOG_TAG, "Too many pin classes on definition %d \"%s\"", is_input, name );
        return;
    }
    PinClass *pin_class = &pin_classes->classes[ pin_classes->used_classes++ ];
    pin_class->name = name;
    pin_class->max_pin_count = max_pin_count;
    pin_class->used_pin_count = 0;
    pin_class->level_type = level_type;
    pin_class->pins = malloc( sizeof( Pin ) * max_pin_count );
}

static void set_pin_state( Pin *output_pin, bool enabled ) {
    output_pin->state = enabled;
    bool level =
            ( enabled && output_pin->level_type == high_is_on ) || ( !enabled && output_pin->level_type == low_is_on );
    gpio_set_level( output_pin->pin, level );
}

static void set_pin_name( Pin *pin, char *name ) {
    if ( pin->name != NULL) {
        free( pin->name );
    }
    pin->name = strdup( name );
}

void gpio_add_pin_with_allocated_name( bool is_input, int class, gpio_num_t pin, char *name, PinLevelType level_type,
                                       uint64_t *pin_bit_mask ) {
    if ( name == NULL) {
        name = strdup( "---" );
    }
    if ( !gpio_is_valid_class( is_input, class )) {
        ESP_LOGE( LOG_TAG, "Adding pin %d/%d %d \"%s\"", is_input, class, pin, name );
        return;
    }
    PinClass *pin_class = &pin_definitions[ is_input ].classes[ class ];

    ESP_LOGI( LOG_TAG, "Adding pin %d/%d/%d %d \"%s\"", is_input, class, pin_class->used_pin_count, pin, name );
    if ( pin_class->used_pin_count == pin_class->max_pin_count ) {
        ESP_LOGE( LOG_TAG, "Too many pins on class %d/%d \"%s\"", is_input, class, name );
        return;
    }
    Pin *output_pin = &pin_class->pins[ pin_class->used_pin_count++ ];
    output_pin->name = name;
    output_pin->pin = pin;
    output_pin->level_type = level_type != inherit ? level_type : pin_class->level_type;

    *pin_bit_mask |= ( 1ULL << pin );
    set_pin_state( output_pin, false );
}

void gpio_add_pin( bool is_input, int class, gpio_num_t pin, char *name, PinLevelType level_type,
                   uint64_t *pin_bit_mask ) {
    name = name != NULL ? strdup( name ) : NULL;
    gpio_add_pin_with_allocated_name( is_input, class, pin, name, level_type, pin_bit_mask );
}

static void add_input_pins() {
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    gpio_define_input_pins_callback( &io_conf );

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    // io_conf.pin_bit_mask = set above
    io_conf.pull_down_en = PIN_DISABLED;
    io_conf.pull_up_en = PIN_DISABLED;
    gpio_config( &io_conf );

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;

    io_conf.pull_down_en = PIN_DISABLED;
    io_conf.pull_up_en = PIN_ENABLED;
    gpio_config( &io_conf );

    //install gpio isr service
    gpio_install_isr_service( ESP_INTR_FLAG_DEFAULT );

    gpio_task_init( gpio_changed_callback );
    PinClasses *input_classes = &pin_definitions[ INPUTS ];
    for ( int input_class_index = 0; input_class_index < input_classes->used_classes; input_class_index++ ) {
        PinClass *input_class = &input_classes->classes[ input_class_index ];
        for ( int input_pin = 0; input_pin < input_class->used_pin_count; input_pin++ ) {
            gpio_task_add( input_class->pins[ input_pin ].pin );
        }
    }
}

static void add_output_pins() {
    gpio_config_t io_conf = {};

    /* does not work to get pin 26 work as output
    // Disable DAC1
    REG_CLR_BIT( RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_XPD_DAC );
    REG_SET_BIT( RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC_XPD_FORCE );
    // Disable DAC2
    REG_CLR_BIT( RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_XPD_DAC );
    REG_SET_BIT( RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC_XPD_FORCE );
     */

    gpio_define_output_pins_callback( &io_conf );
}

void gpio_init() {
    add_input_pins();
    add_output_pins();
}

int gpio_get_number_of_classes( bool is_input ) {
    return pin_definitions[ is_input ].used_classes;
}

char *gpio_get_class_name( bool is_input, int class ) {
    if ( !gpio_is_valid_class( is_input, class )) {
        return "wrong class";
    }
    return pin_definitions[ is_input ].classes[ class ].name;
}

int gpio_get_number_of_pins( bool is_input, int class ) {
    if ( !gpio_is_valid_class( is_input, class )) {
        return -1;
    }
    return pin_definitions[ is_input ].classes[ class ].max_pin_count;
}

bool gpio_get_pin_state( bool is_input, int class, int index ) {
    if ( !gpio_is_valid_index( is_input, class, index )) {
        return -1;
    }
    Pin *pin = &pin_definitions[ is_input ].classes[ class ].pins[ index ];
    return is_input
           ? gpio_get_level( pin->pin )
           : pin->state;
}

char *gpio_get_pin_name( bool is_input, int class, int index ) {
    if ( !gpio_is_valid_index( is_input, class, index )) {
        return NULL;
    }
    return pin_definitions[ is_input ].classes[ class ].pins[ index ].name;
}

void gpio_set_pin_state( bool is_input, int class, int index, bool state ) {
    if ( is_input || !gpio_is_valid_index( is_input, class, index )) {
        return;
    }
    set_pin_state( &pin_definitions[ is_input ].classes[ class ].pins[ index ], state );
}

void gpio_set_pin_name( bool is_input, int class, int index, char *name ) {
    if ( !gpio_is_valid_index( is_input, class, index )) {
        return;
    }
    set_pin_name( &pin_definitions[ is_input ].classes[ class ].pins[ index ], name );
}

