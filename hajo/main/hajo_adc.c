#include "hajo_adc.h"
#include "n2k_png.h"
#include "lib/adc.h"

uint32_t convert_battery_voltage( uint32_t raw_value ) {
    return raw_value;
}

uint32_t convert_fluid_level( uint32_t raw_value ) {
    return raw_value;
}

void hajo_adc_main( bool is_battery ) {
    if ( is_battery ) {
        adc_add_channel( 0, "motor", 0, 0, convert_battery_voltage );
        adc_add_channel( 1, "munka1", 1, 0, convert_battery_voltage );
        adc_add_channel( 2, "munka2", 2, 0, convert_battery_voltage );
    } else {
        adc_add_channel( 4, "uzemanyag", 0, N2K_TANK_TYPE_FUEL, convert_fluid_level );
        adc_add_channel( 5, "viz bal", 1, N2K_TANK_TYPE_WATER, convert_fluid_level );
        adc_add_channel( 6, "viz jobb", 2, N2K_TANK_TYPE_WATER, convert_fluid_level );
    }
    adc_main();
}
