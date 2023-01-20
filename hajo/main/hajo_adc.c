#include "hajo_adc.h"
#include "n2k_png.h"
#include "lib/adc.h"

static int battery_rmes = 6350;
static int battery_rtop = 93400;
static double battery_offset = -140;
//static int battery_rmes = 16900;
//static int battery_rtop = 316000;
static double battery_multiplier;

static double fluid_u = 3.3;
static double fluid_rtop = 634;

uint32_t convert_battery_voltage( uint32_t raw_value ) {
    // U=(Rtop+Rmes)/Rmes*Umes
    return (uint32_t) ( raw_value * battery_multiplier + battery_offset );
}

uint32_t convert_fluid_level( uint32_t raw_value ) {
    // Rmes=Rtop/(U/Umes-1)
    // 0% = 2 Ohm, 100% = 180 Ohm
    double value = fluid_rtop / ( fluid_u / raw_value - 1 );
    if ( value < 0 ) {
        value = 0;
    }
    return (uint32_t) value;
}

void hajo_adc_main( bool is_battery ) {
    if ( is_battery ) {
        battery_multiplier = ( battery_rmes + battery_rtop ) / (double) battery_rmes;
        adc_add_channel( 0, "motor", 0, 0, convert_battery_voltage );
        adc_add_channel( 1, "munka1", 1, 0, convert_battery_voltage );
        adc_add_channel( 2, "munka2", 2, 0, convert_battery_voltage );
    } else {
        adc_add_channel( 4, "uzemanyag", 0, N2K_TANK_TYPE_FUEL, convert_fluid_level );
        adc_add_channel( 5, "viz bal", 0, N2K_TANK_TYPE_WATER, convert_fluid_level );
        adc_add_channel( 6, "viz jobb", 1, N2K_TANK_TYPE_WATER, convert_fluid_level );
    }
    adc_main();
}
