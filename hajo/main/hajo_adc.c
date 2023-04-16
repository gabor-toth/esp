#include "hajo_adc.h"
#include "lib/nmea2000/n2k_png.h"
#include "lib/adc.h"

static int battery_rmes = 16900;
static int battery_rtop = 316000;
static int battery_offset = 0;
static double battery_multiplier;

static double fluid_u = 3.32;
static double fluid_rtop = 680;
//static double fluid_rtop = 634;
static double fluid_rmes_min = 2;
static double fluid_rmes_max = 180;

void convert_battery_voltage( uint32_t raw_value, uint32_t *display_value, uint32_t *correction ) {
    // U=(Rtop+Rmes)/Rmes*Umes
    *correction = battery_offset;
    double value = raw_value * battery_multiplier;
    if ( raw_value <= 20 ) {
        value = 0.0;
        *correction = 0;
    } else {
        value += battery_offset;
    }
    if ( value < 0.0 ) {
        value = 0.0;
    }
    *display_value = (uint32_t) value;
}

void convert_fluid_level( uint32_t raw_value, uint32_t *display_value, uint32_t *correction ) {
    // Rmes=Rtop/(U/Umes-1)
    // 0% = 2 Ohm, 100% = 180 Ohm
    double rmes = fluid_rtop / ( fluid_u * 1000 / raw_value - 1 );
    uint32_t value;
    if ( rmes <= fluid_rmes_min ) {
        value = 0;
    } else if ( rmes >= fluid_rmes_max ) {
        value = 100;
    } else {
        value = (uint32_t) (( rmes - fluid_rmes_min ) / ( fluid_rmes_max - fluid_rmes_min ) * 100 );
    }
    *display_value = value;
    *correction = 0;
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
    adc_main( false );
}
