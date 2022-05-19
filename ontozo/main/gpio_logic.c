#include <stdio.h>
#include <string.h>
#include <driver/gpio.h>
#include <soc/rtc_io_reg.h>
#include "lib/gpio_task.h"
#include "lib/common.h"
#include "config.h"
#include "gpio_logic.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define reached( X ) ((X)!=0)

static void gpio_changed( uint32_t io_num, int state );

static void set_initial_pump_states();

typedef struct {
    gpio_num_t pin;
    int state;
    char *name;
} OutputPin;

#define PUMP_MAIN     0
#define PUMP_REFILL   1
#define PUMP_MAX      2
#define IS_PUMP_VALID( X )    ((X)>=0 && (X) <PUMP_MAX)

static gpio_num_t level_pins[] = {
        GPIO_INPUT_LEVEL_1,
        GPIO_INPUT_LEVEL_2,
        GPIO_INPUT_LEVEL_3,
        GPIO_INPUT_LEVEL_4
};

static OutputPin pumps[PUMP_MAX] = {
        {
                .pin = GPIO_OUTPUT_PUMP_MAIN,
                .name = "öntöző"
        },
        {
                .pin = GPIO_OUTPUT_PUMP_REFILL,
                .name = "kút"
        }
};

#define ZONE_MAX    8
#define IS_ZONE_VALID( X )    ((X)>=0 && (X) <zone_count)

static OutputPin zones[ZONE_MAX] = {
        {
                .pin = GPIO_OUTPUT_ZONE_1,
                .name = "fű nagy"
        },
        {
                .pin = GPIO_OUTPUT_ZONE_2,
                .name = "fű elöl"
        },
        {
                .pin = GPIO_OUTPUT_ZONE_3,
                .name = "fű hátul"
        },
        {
                .pin = GPIO_OUTPUT_ZONE_4,
                .name = "hátsó kiskert"
        },
        {
                .pin = GPIO_OUTPUT_ZONE_5,
                .name = "első kiskert"
        },
        {
                .pin = GPIO_OUTPUT_ZONE_6,
                .name = "veteményes"
        },
        {
                .pin = GPIO_OUTPUT_ZONE_7,
                .name = "ribizli"
        },
        {
                .pin = GPIO_OUTPUT_ZONE_8,
                .name = "nem használt"
        }
};
static int zone_count = 7;

static void print_state() {
    printf( "levels: 4-%d%d%d%d-1 pumps: %c%c zones: %c%c%c%c%c%c%c%c\n",
            gpio_get_level( GPIO_INPUT_LEVEL_4 ),
            gpio_get_level( GPIO_INPUT_LEVEL_3 ),
            gpio_get_level( GPIO_INPUT_LEVEL_2 ),
            gpio_get_level( GPIO_INPUT_LEVEL_1 ),
            pumps[ PUMP_MAIN ].state ? 'M' : 'm',
            pumps[ PUMP_REFILL ].state ? 'R' : 'r',
            zones[ 0 ].state,
            zones[ 1 ].state,
            zones[ 2 ].state,
            zones[ 3 ].state,
            zones[ 4 ].state,
            zones[ 5 ].state,
            zones[ 6 ].state,
            zones[ 7 ].state );
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

    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    for ( int i = 0; i < PUMP_MAX; i++ ) {
        gpio_set_level( pumps[ i ].pin, PUMP_DISABLED );
        io_conf.pin_bit_mask |= ( 1ULL << pumps[ i ].pin );
        pumps[ i ].name = strdup( pumps[ i ].name );
    }
    for ( int i = 0; i < ZONE_MAX; i++ ) {
        gpio_set_level( zones[ i ].pin, ZONE_DISABLED );
        io_conf.pin_bit_mask |= ( 1ULL << zones[ i ].pin );
        zones[ i ].name = strdup( zones[ i ].name );
    }

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

static void set_pump_state( int pump, int enabled ) {
    pumps[ pump ].state = enabled;
    gpio_set_level( pumps[ pump ].pin, enabled ? PUMP_ENABLED : PUMP_DISABLED );
}

static void set_zone_state( int zone, int enabled ) {
    zones[ zone ].state = enabled;
    gpio_set_level( zones[ zone ].pin, enabled ? ZONE_ENABLED : ZONE_DISABLED );
}

static void set_initial_pump_states() {
    int level0 = gpio_get_level( GPIO_INPUT_LEVEL_1 );
    int level1 = gpio_get_level( GPIO_INPUT_LEVEL_2 );
    int level2 = gpio_get_level( GPIO_INPUT_LEVEL_3 );
    int level3 = gpio_get_level( GPIO_INPUT_LEVEL_4 );

    set_pump_state( PUMP_MAIN,
                    reached( level0 )
                    && reached( level1 ));
    set_pump_state( PUMP_REFILL,
                    !reached( level3 )
                    || !reached( level2 )
                    || !reached( level1 )
                    || !reached( level0 ));
    print_state();
}

static void gpio_changed( uint32_t io_num, int state ) {
    printf( "GPIO[%d] = %d\n", io_num, state );

    if ( io_num == GPIO_INPUT_LEVEL_4 ) {
        if ( state == ENABLED ) {
            set_pump_state( PUMP_REFILL, false );
        } else {
            set_pump_state( PUMP_REFILL, true );
        }
    } else if ( io_num == GPIO_INPUT_LEVEL_3 ) {
        // no change for this sensor
    } else if ( io_num == GPIO_INPUT_LEVEL_2 ) {
        if ( state == ENABLED ) {
            set_pump_state( PUMP_MAIN, true );
        }
    } else if ( io_num == GPIO_INPUT_LEVEL_1 ) {
        if ( state == DISABLED ) {
            set_pump_state( PUMP_MAIN, false );
        }
    } else if ( io_num == GPIO_INPUT_BUTTON_START ) {
        set_pump_state( PUMP_MAIN, true );
    } else if ( io_num == GPIO_INPUT_BUTTON_STOP ) {
        set_pump_state( PUMP_MAIN, false );
    } else {
        fprintf(stderr, "Unhandled gpio %d (state %d)!\n", io_num, state );
    }

    print_state();
}

// levels

int gpio_get_number_of_levels() {
    return sizeof level_pins / sizeof level_pins[ 0 ];
}

int gpio_get_level_state( int level ) {
    return gpio_get_level( level_pins[ level ] );
}

// common

void set_name( OutputPin *pin, char *name ) {
    if ( pin->name != NULL) {
        free( pin->name );
    }
    pin->name = strdup( name );
}


// zones

int gpio_get_number_of_zones() {
    return zone_count;
}

int gpio_is_zone_valid( int zone ) {
    return IS_ZONE_VALID( zone );
}

int gpio_get_zone_state( int zone ) {
    if ( !IS_ZONE_VALID( zone )) {
        return -1;
    }
    return zones[ zone ].state;
}

char *gpio_get_zone_name( int zone ) {
    return zones[ zone ].name;
}

void gpio_set_zone_state( int zone, int state ) {
    if ( !IS_ZONE_VALID( zone )) {
        return;
    }
    set_zone_state( zone, state );
}

void gpio_set_zone_name( int zone, char *name ) {
    if ( !IS_ZONE_VALID( zone )) {
        return;
    }
    set_name( &zones[ zone ], name );
}

// pumps

int gpio_get_number_of_pumps() {
    return PUMP_MAX;
}

int gpio_get_pump_state( int pump ) {
    if ( !IS_PUMP_VALID( pump )) {
        return -1;
    }
    return pumps[ pump ].state;
}

char *gpio_get_pump_name( int pump ) {
    if ( !IS_PUMP_VALID( pump )) {
        return "invalid";
    }
    return pumps[ pump ].name;
}

void gpio_set_pump_state( int pump, int state ) {
    if ( !IS_PUMP_VALID( pump )) {
        return;
    }

}

void gpio_set_pump_name( int pump, char *name ) {
    if ( !IS_PUMP_VALID( pump )) {
        return;
    }
    set_name( &pumps[ pump ], name );
}
