#include <stdio.h>
#include <string.h>
#include <driver/gpio.h>
#include <soc/rtc_io_reg.h>
#include <esp_log.h>
#include "lib/gpio_task.h"
#include "lib/common.h"
#include "config.h"
#include "gpio_logic.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define reached( X ) ((X)!=0)

static void gpio_changed( uint32_t io_num, int state );

static void set_initial_pump_states();

#define PUMP_MAIN     0
#define PUMP_REFILL   1

static gpio_num_t level_pins[] = {
        GPIO_INPUT_LEVEL_1,
        GPIO_INPUT_LEVEL_2,
        GPIO_INPUT_LEVEL_3,
        GPIO_INPUT_LEVEL_4
};

typedef enum {
    normal, inverted, inherit = -1
} PinLevelType;

typedef struct {
    gpio_num_t pin;
    char *name;
    PinLevelType level_type;
    bool state;
} OutputPin;

typedef struct {
    char *name;
    int max_pin_count;
    int used_pin_count;
    OutputPin *pins;
    PinLevelType level_type;
} OutputClass;

#define MAX_CLASSES 4

static OutputClass outputs[MAX_CLASSES];
static int output_classes_count = 0;

#define ZONE_MAX    8
#define IS_ZONE_VALID( X )    ((X)>=0 && (X) <zone_count)

static void print_state() {
    printf( "levels: 4-%d%d%d%d-1 pumps: %c%c zones: %d%d%d%d%d%d%d%d\n",
            gpio_get_level( GPIO_INPUT_LEVEL_4 ),
            gpio_get_level( GPIO_INPUT_LEVEL_3 ),
            gpio_get_level( GPIO_INPUT_LEVEL_2 ),
            gpio_get_level( GPIO_INPUT_LEVEL_1 ),
            outputs[ PUMPS ].pins[ PUMP_MAIN ].state ? 'M' : 'm',
            outputs[ PUMPS ].pins[ PUMP_REFILL ].state ? 'R' : 'r',
            outputs[ ZONES ].pins[ 0 ].state,
            outputs[ ZONES ].pins[ 1 ].state,
            outputs[ ZONES ].pins[ 2 ].state,
            outputs[ ZONES ].pins[ 3 ].state,
            outputs[ ZONES ].pins[ 4 ].state,
            outputs[ ZONES ].pins[ 5 ].state,
            outputs[ ZONES ].pins[ 6 ].state,
            outputs[ ZONES ].pins[ 7 ].state );
}

static void add_output_class( char *name, int max_pin_count, PinLevelType level_type ) {
    OutputClass *output_class = &outputs[ output_classes_count++ ];
    output_class->name = name;
    output_class->max_pin_count = max_pin_count;
    output_class->used_pin_count = 0;
    output_class->level_type = level_type;
    output_class->pins = malloc( sizeof( OutputPin ) * max_pin_count );
}

static void set_pin_state( OutputPin *output_pin, bool enabled ) {
    output_pin->state = enabled;
    bool level = ( enabled && output_pin->level_type == normal ) || ( !enabled && output_pin->level_type == inverted );
    gpio_set_level( output_pin->pin, level );
}

void set_pin_name( OutputPin *pin, char *name ) {
    if ( pin->name != NULL) {
        free( pin->name );
    }
    pin->name = strdup( name );
}

static void add_output_pin( int class, gpio_num_t pin, char *name, PinLevelType level_type, uint64_t *pin_bit_mask ) {
    OutputClass *output_class = &outputs[ class ];

    OutputPin *output_pin = &output_class->pins[ output_class->used_pin_count++ ];
    output_pin->name = name != NULL ? strdup( name ) : NULL;
    output_pin->pin = pin;
    output_pin->level_type = level_type != inherit ? level_type : output_class->level_type;

    *pin_bit_mask |= ( 1ULL << pin );
    set_pin_state( output_pin, false );
}

void gpio_init() {

    /* does not work to get pin 26 work as output
    // Disable DAC1
    REG_CLR_BIT( RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_XPD_DAC );
    REG_SET_BIT( RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC_XPD_FORCE );
    // Disable DAC2
    REG_CLR_BIT( RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_XPD_DAC );
    REG_SET_BIT( RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC_XPD_FORCE );
     */

    add_output_class( "pumps", 2, normal );
    add_output_class( "zones", 8, inverted );

    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    add_output_pin( PUMPS, GPIO_OUTPUT_PUMP_MAIN, "öntöző", inherit, &io_conf.pin_bit_mask );
    add_output_pin( PUMPS, GPIO_OUTPUT_PUMP_REFILL, "kút", inherit, &io_conf.pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_1, "fű nagy", inherit, &io_conf.pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_2, "fű elöl", inherit, &io_conf.pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_3, "fű hátul", inherit, &io_conf.pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_4, "hátsó kiskert", inherit, &io_conf.pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_5, "első kiskert", inherit, &io_conf.pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_6, "veteményes", inherit, &io_conf.pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_7, "ribizli", inherit, &io_conf.pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_8, NULL, inherit, &io_conf.pin_bit_mask );

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    // io_conf.pin_bit_mask = set above
    io_conf.pull_down_en = DISABLED;
    io_conf.pull_up_en = DISABLED;
    gpio_config( &io_conf );

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;

    io_conf.pin_bit_mask =
            ( 1ULL << GPIO_INPUT_LEVEL_1 ) |
            ( 1ULL << GPIO_INPUT_LEVEL_2 ) |
            ( 1ULL << GPIO_INPUT_LEVEL_3 ) |
            ( 1ULL << GPIO_INPUT_LEVEL_4 ) |
            ( 1ULL << GPIO_INPUT_BUTTON_START ) |
            ( 1ULL << GPIO_INPUT_BUTTON_STOP ) |
            0;
    io_conf.pull_down_en = DISABLED;
    io_conf.pull_up_en = ENABLED;
    gpio_config( &io_conf );

    //install gpio isr service
    gpio_install_isr_service( ESP_INTR_FLAG_DEFAULT );

    gpio_task_init( gpio_changed );
    gpio_task_add( GPIO_INPUT_LEVEL_1 );
    gpio_task_add( GPIO_INPUT_LEVEL_2 );
    gpio_task_add( GPIO_INPUT_LEVEL_3 );
    gpio_task_add( GPIO_INPUT_LEVEL_4 );
    gpio_task_add( GPIO_INPUT_BUTTON_START );
    gpio_task_add( GPIO_INPUT_BUTTON_STOP );

    set_initial_pump_states();
    print_state();
}

static void set_initial_pump_states() {
    int level0 = gpio_get_level( GPIO_INPUT_LEVEL_1 );
    int level1 = gpio_get_level( GPIO_INPUT_LEVEL_2 );
    int level2 = gpio_get_level( GPIO_INPUT_LEVEL_3 );
    int level3 = gpio_get_level( GPIO_INPUT_LEVEL_4 );

    set_pin_state( &outputs[ PUMPS ].pins[ PUMP_MAIN ],
                   reached( level0 )
                   && reached( level1 ));
    set_pin_state( &outputs[ PUMPS ].pins[ PUMP_REFILL ],
                   !reached( level3 )
                   || !reached( level2 )
                   || !reached( level1 )
                   || !reached( level0 ));
    print_state();
}

static void gpio_changed( uint32_t io_num, int state ) {
    printf( "GPIO[%d] = %d\n", io_num, state );

    bool refill_state = outputs[ PUMPS ].pins[ PUMP_REFILL ].state;
    bool main_state = outputs[ PUMPS ].pins[ PUMP_MAIN ].state;

    if ( io_num == GPIO_INPUT_LEVEL_4 ) {
        if ( state == ENABLED ) {
            refill_state = false;
        } else {
            refill_state = true;
        }
    } else if ( io_num == GPIO_INPUT_LEVEL_3 ) {
        // no change for this sensor
    } else if ( io_num == GPIO_INPUT_LEVEL_2 ) {
        if ( state == ENABLED ) {
            main_state = true;
        }
    } else if ( io_num == GPIO_INPUT_LEVEL_1 ) {
        if ( state == DISABLED ) {
            main_state = false;
        }
    } else if ( io_num == GPIO_INPUT_BUTTON_START ) {
        main_state = true;
    } else if ( io_num == GPIO_INPUT_BUTTON_STOP ) {
        main_state = false;
    } else {
        fprintf(stderr, "Unhandled gpio %d (state %d)!\n", io_num, state );
    }

    set_pin_state( &outputs[ PUMPS ].pins[ PUMP_MAIN ], main_state );
    set_pin_state( &outputs[ PUMPS ].pins[ PUMP_REFILL ], refill_state );
    print_state();
}

// inputs

int gpio_get_number_of_levels() {
    return sizeof level_pins / sizeof level_pins[ 0 ];
}

int gpio_get_level_state( int level ) {
    return gpio_get_level( level_pins[ level ] );
}

// outputs

int gpio_get_number_of_output_classes() {
    return output_classes_count;
}

char *gpio_get_class_name( int class ) {
    if ( class < 0 || class >= output_classes_count ) {
        return "-1";
    }
    return outputs[ class ].name;
}

int gpio_get_number_of_output_pins( int class ) {
    if ( class < 0 || class >= output_classes_count ) {
        return -1;
    }
    return outputs[ class ].max_pin_count;
}

bool gpio_is_valid_output_index( int class, int index ) {
    return class >= 0
           && class < output_classes_count
           && index >= 0
           && index < outputs[ class ].max_pin_count;
}

bool gpio_get_output_pin_state( int class, int index ) {
    if ( !gpio_is_valid_output_index( class, index )) {
        return -1;
    }
    return outputs[ class ].pins[ index ].state;
}

char *gpio_get_output_pin_name( int class, int index ) {
    if ( !gpio_is_valid_output_index( class, index )) {
        return NULL;
    }
    return outputs[ class ].pins[ index ].name;
}

void gpio_set_output_pin_state( int class, int index, bool state ) {
    if ( !gpio_is_valid_output_index( class, index )) {
        return;
    }
    set_pin_state( &outputs[ class ].pins[ index ], state );
}

void gpio_set_output_pin_name( int class, int index, char *name ) {
    if ( !gpio_is_valid_output_index( class, index )) {
        return;
    }
    set_pin_name( &outputs[ class ].pins[ index ], name );
}
