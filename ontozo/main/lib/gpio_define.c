#include <string.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "lib/gpio_task.h"
#include "lib/nvs_main.h"
#include "gpio_define.h"
#include "gpio_json.h"

static const char *LOG_TAG = "gpio";

#define ESP_INTR_FLAG_DEFAULT 0

#define reached( X ) ((X)!=0)

#define PUMP_MAIN     0
#define PUMP_REFILL   1

#define MAX_PIN_CLASSES 4

static nvs_handle_t nvs_storage_handle;

typedef struct {
    gpio_num_t pin;
    char *name;
    PinLevelType level_type;
    bool state;
    bool is_manual;
    int delay_ms_going_low;
    int delay_ms_going_high;
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

static void get_nvs_key( bool is_input, int class, int index, char *buffer, size_t buffer_size ) {
    snprintf( buffer, buffer_size, "%s.%d", gpio_get_class_name( is_input, class ), index + 1 );
}

static bool is_valid_class( bool is_input, int class ) {
    if ( class >= 0 && class < pin_definitions[ is_input ].used_classes ) {
        return true;
    }
    ESP_LOGE( LOG_TAG, "Wrong pin class %d/%d >= %d/%d",
              is_input, class,
              sizeof( pin_definitions ) / sizeof( pin_definitions[ 0 ] ), pin_definitions[ is_input ].used_classes );
    return false;
}

bool gpio_is_valid_index( bool is_input, int class, int index ) {
    if ( !is_valid_class( is_input, class )) {
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
    ESP_LOGI( LOG_TAG, "set pin %d to %d", output_pin->pin, level );
    gpio_set_level( output_pin->pin, level );
}

static void read_nvs_or_default( bool is_input, int class, int index, PinData *pin_data ) {
    char nvs_key[256];

    memset( pin_data, 0, sizeof( PinData ));
    get_nvs_key( is_input, class, index, nvs_key, sizeof( nvs_key ));
    char *json_string = nvs_read_string( nvs_storage_handle, nvs_key );
    if ( json_string == NULL) {
        pin_data->name = strdup( nvs_key );
        return;
    }

    gpio_data_from_json_string( json_string, pin_data );
    free( json_string );
}

int gpio_add_pin( bool is_input, int class, gpio_num_t gpio_pin, PinLevelType level_type, uint64_t *pin_bit_mask ) {
    if ( !is_valid_class( is_input, class )) {
        ESP_LOGE( LOG_TAG, "Adding pin %d/%d %d", is_input, class, gpio_pin );
        return -1;
    }

    PinClass *pin_class = &pin_definitions[ is_input ].classes[ class ];

    ESP_LOGI( LOG_TAG, "Adding pin %d/%d/%d %d", is_input, class, pin_class->used_pin_count, gpio_pin );
    if ( pin_class->used_pin_count == pin_class->max_pin_count ) {
        ESP_LOGE( LOG_TAG, "Too many pins on class %d/%d", is_input, class );
        return -1;
    }
    int index = pin_class->used_pin_count++;

    PinData pin_data;
    read_nvs_or_default( is_input, class, index, &pin_data );

    Pin *pin = &pin_class->pins[ index ];
    pin->name = pin_data.name;
    pin->is_manual = pin_data.is_manual;
    pin->pin = gpio_pin;
    pin->level_type = level_type != inherit ? level_type : pin_class->level_type;

    *pin_bit_mask |= ( 1ULL << gpio_pin );
    if ( !is_input ) {
        set_pin_state( pin, false );
    }
    return index;
}

static void add_input_pins( void *user_context ) {
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    gpio_define_input_pins_callback( &io_conf, user_context );

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
            Pin *pin = &input_class->pins[ input_pin ];
            gpio_task_add( pin->pin, pin->delay_ms_going_low, pin->delay_ms_going_high );
        }
    }
}

static void add_output_pins( void *user_context ) {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = PIN_DISABLED;
    io_conf.pull_up_en = PIN_DISABLED;

    /* does not work to get pin 26 work as output
    // Disable DAC1
    REG_CLR_BIT( RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_XPD_DAC );
    REG_SET_BIT( RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC_XPD_FORCE );
    // Disable DAC2
    REG_CLR_BIT( RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_XPD_DAC );
    REG_SET_BIT( RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC_XPD_FORCE );
     */

    gpio_define_output_pins_callback( &io_conf, user_context );

    gpio_config( &io_conf );
}

void gpio_init( void *user_context ) {
    nvs_storage_handle = nvs_open_storage();
    add_input_pins( user_context );
    add_output_pins( user_context );
    nvs_close_storage( nvs_storage_handle );
    nvs_storage_handle = 0;
}

int gpio_get_number_of_classes( bool is_input ) {
    return pin_definitions[ is_input ].used_classes;
}

char *gpio_get_class_name( bool is_input, int class ) {
    if ( !is_valid_class( is_input, class )) {
        return "wrong class";
    }
    return pin_definitions[ is_input ].classes[ class ].name;
}

int gpio_get_number_of_pins( bool is_input, int class ) {
    if ( !is_valid_class( is_input, class )) {
        return -1;
    }
    return pin_definitions[ is_input ].classes[ class ].max_pin_count;
}

bool gpio_get_pin_state( bool is_input, int class, int index ) {
    if ( !gpio_is_valid_index( is_input, class, index )) {
        return -1;
    }
    Pin *pin = &pin_definitions[ is_input ].classes[ class ].pins[ index ];
    if ( !is_input ) {
        return pin->state;
    }
    int state = gpio_get_level( pin->pin );
    return ( pin->level_type == high_is_on ) == state;
}

bool gpio_get_pin_data( bool is_input, int class, int index, PinData *pin_data ) {
    if ( !gpio_is_valid_index( is_input, class, index )) {
        return false;
    }
    Pin *pin = &pin_definitions[ is_input ].classes[ class ].pins[ index ];
    pin_data->name = pin->name;
    pin_data->is_manual = pin->is_manual;
    pin_data->state = gpio_get_pin_state( is_input, class, index );
    return true;
}

void gpio_set_pin_state( bool is_input, int class, int index, bool state ) {
    if ( is_input || !gpio_is_valid_index( is_input, class, index )) {
        return;
    }

    Pin *pin = &pin_definitions[ is_input ].classes[ class ].pins[ index ];
    if ( !pin->is_manual ) {
        set_pin_state( pin, state );
    }
}

void gpio_set_pin_state_forced( bool is_input, int class, int index, bool state ) {
    if ( is_input || !gpio_is_valid_index( is_input, class, index )) {
        return;
    }
    set_pin_state( &pin_definitions[ is_input ].classes[ class ].pins[ index ], state );
}

bool gpio_set_pin_data( bool is_input, int class, int index, PinData *pin_data ) {
    if ( !gpio_is_valid_index( is_input, class, index )) {
        return false;
    }
    Pin *pin = &pin_definitions[ is_input ].classes[ class ].pins[ index ];
    bool write_to_nvs = false;
    if ( pin_data->name != NULL && pin_data->name != pin->name ) {
        if ( pin->name != NULL) {
            free( pin->name );
        }
        pin->name = strdup( pin_data->name );
        write_to_nvs = true;
    }
    if ( pin_data->is_manual != pin->is_manual ) {
        pin->is_manual = pin_data->is_manual;
        write_to_nvs = true;
    }
    if ( write_to_nvs ) {
        char *json_string = gpio_data_to_json_string( pin_data );
        char nvs_key[256];
        get_nvs_key( is_input, class, index, nvs_key, sizeof nvs_key );
        nvs_open_and_write_string( nvs_key, json_string );
        free( json_string );
    }
    return true;
}

void gpio_set_delays( bool is_input, int class, int index, int delay_ms_going_low, int delay_ms_going_high ) {
    if ( !gpio_is_valid_index( is_input, class, index )) {
        return;
    }
    Pin *pin = &pin_definitions[ is_input ].classes[ class ].pins[ index ];
    pin->delay_ms_going_high = delay_ms_going_high;
    pin->delay_ms_going_low = delay_ms_going_low;
}
