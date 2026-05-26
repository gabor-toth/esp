#include "display_main.h"
#include "display_meter.h"
#include "esp_log.h"
#include "lvgl/lvgl.h"

#define USE_ANIMATION   0

static const char *LOG = "displ_meter";

typedef struct {
    int index;
    lv_style_t *current_style;
} user_data_t;

static bool initialized = false;
static lv_style_t style_bar_background;
static lv_style_t style_image;
static lv_style_t style_indic_battery;
static lv_style_t style_indic_battery_charge;
static lv_style_t style_indic_battery_deplete;
static lv_style_t style_indic_battery_empty;
static lv_style_t style_indic_water;
static lv_style_t style_indic_water_deplete;
static lv_style_t style_indic_water_empty;
static lv_style_t style_indic_fuel;
static lv_style_t style_indic_fuel_empty;
static lv_style_t style_label;

static lv_color_t color_indic_empty = LV_COLOR_MAKE( 0xF4, 0x43, 0x36 );
static lv_color_t color_label = LV_COLOR_MAKE( 255, 255, 255 );

static lv_obj_t *bar_water;
static lv_obj_t *bar_fuel;
static lv_obj_t *bar_battery[ 3 ];

#define SDL_VER_RES 240
#define STYLE_BAR_PADDING 8
#define STYLE_BAR_OUTLINE 4
#define STYLE_BAR_WIDTH 200
#define STYLE_BAR_HEIGHT 20
#define STYLE_BAR_X 34
#define STYLE_BAR_EMPTY_STATE 5
#define STYLE_IMAGE_WIDTH 24
#define STYLE_IMAGE_X (SDL_VER_RES-STYLE_IMAGE_WIDTH-4)
#define BATTERY_VOLTAGE_START       90
#define BATTERY_VOLTAGE_WARN_LOW   108
#define BATTERY_VOLTAGE_DEPLETE    110
#define BATTERY_VOLTAGE_WARN_DEPLETE   (BATTERY_VOLTAGE_DEPLETE+10)
#define BATTERY_VOLTAGE_CHARGE     136
#define BATTERY_VOLTAGE_WARN_HIGH  147
#define BATTERY_VOLTAGE_END        140

LV_IMG_DECLARE( img_battery );
LV_IMG_DECLARE( img_battery_engine );
LV_IMG_DECLARE( img_fuel );
LV_IMG_DECLARE( img_water );

LV_FONT_DECLARE( lv_font_montserrat_12 )

void setup_screen() {
    static lv_style_t style_screen;
    lv_obj_t *screen = lv_scr_act();

    lv_style_init( &style_screen );
    lv_color_t color_bg_top = LV_COLOR_MAKE( 70, 75, 85 );
    lv_color_t color_bg_bottom = LV_COLOR_MAKE( 45, 50, 60 );
    lv_style_set_bg_color( &style_screen, color_bg_top );
    //    lv_style_set_bg_grad_color( &style_screen, color_bg_bottom );
    //    lv_style_set_bg_grad_dir( &style_screen, LV_GRAD_DIR_VER );
    lv_obj_add_style( screen, &style_screen, LV_PART_MAIN );
}

static void set_style( void *bar, lv_style_t *style ) {
    user_data_t *user_data = lv_obj_get_user_data( bar );
    if ( user_data->current_style != NULL ) {
        lv_obj_remove_style( bar, user_data->current_style, LV_PART_INDICATOR );
    }
    lv_obj_add_style( bar, style, LV_PART_INDICATOR );
    user_data->current_style = style;
}

static void setup_bar_background_style() {
    lv_color_t color_bg = LV_COLOR_MAKE( 15, 18, 21 );
    lv_color_t color_outline = LV_COLOR_MAKE( 30, 36, 45 );
    lv_color_t color_shadow = LV_COLOR_MAKE( 5, 10, 15 );

    lv_style_init( &style_bar_background );
    lv_style_set_bg_color( &style_bar_background, color_bg );
    lv_style_set_bg_opa( &style_bar_background, LV_OPA_COVER );
    lv_style_set_pad_all( &style_bar_background, STYLE_BAR_PADDING - STYLE_BAR_OUTLINE );
    lv_style_set_outline_color( &style_bar_background, color_outline );
    lv_style_set_outline_width( &style_bar_background, STYLE_BAR_OUTLINE );
    lv_style_set_shadow_color( &style_bar_background, color_shadow );
    lv_style_set_shadow_ofs_x( &style_bar_background, 10 );
    lv_style_set_shadow_ofs_y( &style_bar_background, 10 );
    //    lv_style_set_shadow_width(&style_bar_background, 10);
}

void setup_bar_common( int index, lv_obj_t *bar, lv_style_t *style_indic, int y0 ) {
    user_data_t *user_data = calloc( 1, sizeof( user_data_t ) );
    user_data->index = index;
    lv_obj_set_user_data( bar, user_data );

    set_style( bar, style_indic );
    lv_obj_add_style( bar, &style_bar_background, LV_PART_MAIN );

    lv_obj_set_size( bar, STYLE_BAR_HEIGHT + 2 * STYLE_BAR_PADDING - 2 * STYLE_BAR_OUTLINE,
        STYLE_BAR_WIDTH - 2 * STYLE_BAR_OUTLINE );
    lv_obj_set_pos( bar, y0, STYLE_BAR_OUTLINE + STYLE_BAR_PADDING );
}

void set_image_position( lv_obj_t *icon, int y0 ) {
    lv_obj_set_pos( icon, y0 + STYLE_BAR_OUTLINE, STYLE_IMAGE_X );
}

lv_obj_t *setup_water_bar( lv_obj_t *parent, int y0 ) {
    lv_color_t color_indic_water = LV_COLOR_MAKE( 54, 185, 246 );

    lv_style_init( &style_indic_water );
    lv_style_set_bg_opa( &style_indic_water, LV_OPA_COVER );
    lv_style_set_bg_color( &style_indic_water, color_indic_water );

    lv_style_init( &style_indic_water_deplete );
    lv_style_set_bg_opa( &style_indic_water_deplete, LV_OPA_COVER );
    lv_style_set_bg_color( &style_indic_water_deplete, color_indic_empty );
    //    lv_style_set_bg_grad_color( &style_indic_water_deplete, color_indic_water );
    //    lv_style_set_bg_grad_dir( &style_indic_water_deplete, LV_GRAD_DIR_VER );
    ////    lv_style_set_bg_main_stop(&style_indic_water_25, 0);
    //    lv_style_set_bg_grad_stop( &style_indic_water_deplete, 20 );

    lv_style_init( &style_indic_water_empty );
    lv_style_set_bg_opa( &style_indic_water_empty, LV_OPA_COVER );
    lv_style_set_bg_color( &style_indic_water_empty, color_indic_empty );

    lv_obj_t *bar = lv_bar_create( parent );
    setup_bar_common( 0, bar, &style_indic_water, y0 );

    lv_bar_set_range( bar, 0, 100 + STYLE_BAR_EMPTY_STATE );
    lv_bar_set_value( bar, 0, LV_ANIM_OFF );

    lv_obj_t *icon = lv_img_create( parent );
    lv_img_set_src( icon, &img_water );
    lv_obj_add_style( icon, &style_image, LV_PART_MAIN );
    set_image_position( icon, y0 );

    return bar;
}

lv_obj_t *setup_fuel_bar( lv_obj_t *parent, int y0 ) {
    lv_color_t color_indic_fuel = LV_COLOR_MAKE( 152, 152, 21 );

    lv_style_init( &style_indic_fuel );
    lv_style_set_bg_opa( &style_indic_fuel, LV_OPA_COVER );
    lv_style_set_bg_color( &style_indic_fuel, color_indic_fuel );

    lv_style_init( &style_indic_fuel_empty );
    lv_style_set_bg_opa( &style_indic_fuel_empty, LV_OPA_COVER );
    //    lv_style_set_bg_color( &style_indic_fuel_empty, color_indic_empty );
    //    lv_style_set_bg_grad_color( &style_indic_fuel_empty, color_indic_fuel );
    //    lv_style_set_bg_grad_dir( &style_indic_fuel_empty, LV_GRAD_DIR_VER );
    ////    lv_style_set_bg_main_stop(&style_indic_fuel_empty, 0);
    //    lv_style_set_bg_grad_stop( &style_indic_fuel_empty, 20 );

    lv_obj_t *bar = lv_bar_create( parent );
    setup_bar_common( 1, bar, &style_indic_fuel, y0 );

    lv_bar_set_range( bar, 0, 100 + STYLE_BAR_EMPTY_STATE );
    lv_bar_set_value( bar, 0, LV_ANIM_OFF );

    lv_obj_t *icon = lv_img_create( parent );
    lv_img_set_src( icon, &img_fuel );
    lv_obj_add_style( icon, &style_image, LV_PART_MAIN );
    set_image_position( icon, y0 );

    return bar;
}

void setup_battery_style() {
    lv_color_t color_indic_battery = LV_COLOR_MAKE( 85, 170, 0 );
    lv_color_t color_indic_battery_warn = LV_COLOR_MAKE( 255, 170, 0 );

    lv_style_init( &style_indic_battery );
    lv_style_set_bg_opa( &style_indic_battery, LV_OPA_COVER );
    lv_style_set_bg_color( &style_indic_battery, color_indic_battery );

    lv_style_init( &style_indic_battery_empty );
    lv_style_set_bg_opa( &style_indic_battery_empty, LV_OPA_COVER );
    lv_style_set_bg_color( &style_indic_battery_empty, color_indic_battery );

    int position;
    lv_style_init( &style_indic_battery_deplete );
    lv_style_set_bg_opa( &style_indic_battery_deplete, LV_OPA_COVER );
    lv_style_set_bg_color( &style_indic_battery_deplete, color_indic_empty );
    //    lv_style_set_bg_grad_color( &style_indic_battery_deplete, color_indic_battery );
    //    lv_style_set_bg_grad_dir( &style_indic_battery_deplete, LV_GRAD_DIR_VER );
    //    position =
    //            255 * ( BATTERY_VOLTAGE_DEPLETE - BATTERY_VOLTAGE_START ) / ( BATTERY_VOLTAGE_END - BATTERY_VOLTAGE_START );
    //    lv_style_set_bg_main_stop( &style_indic_battery_deplete, position - 10 );
    //    lv_style_set_bg_grad_stop( &style_indic_battery_deplete, position + 10 );

    lv_style_init( &style_indic_battery_charge );
    lv_style_set_bg_opa( &style_indic_battery_charge, LV_OPA_COVER );
    lv_style_set_bg_color( &style_indic_battery_charge, color_indic_battery );
    //    lv_style_set_bg_grad_color( &style_indic_battery_charge, color_indic_battery_warn );
    //    lv_style_set_bg_grad_dir( &style_indic_battery_charge, LV_GRAD_DIR_VER );
    //    position =
    //            255 * ( BATTERY_VOLTAGE_CHARGE - BATTERY_VOLTAGE_START ) / ( BATTERY_VOLTAGE_END - BATTERY_VOLTAGE_START );
    //    lv_style_set_bg_main_stop( &style_indic_battery_charge, position - 10 );
    //    lv_style_set_bg_grad_stop( &style_indic_battery_charge, position + 10 );
}

lv_obj_t *setup_battery_bar( lv_obj_t *parent, int y0, int index ) {
    lv_obj_t *bar = lv_bar_create( parent );
    setup_bar_common( index + 2, bar, &style_indic_battery_charge, y0 );

    lv_bar_set_range( bar, BATTERY_VOLTAGE_START, BATTERY_VOLTAGE_END );
    lv_bar_set_value( bar, BATTERY_VOLTAGE_END, LV_ANIM_OFF );

    lv_obj_t *icon = lv_img_create( parent );
    lv_img_set_src( icon, index == 0 ? &img_battery_engine : &img_battery );
    lv_obj_add_style( icon, &style_image, LV_PART_MAIN );
    set_image_position( icon, y0 );

    return bar;
}

void setup_image_style() {
    lv_style_init( &style_image );
    lv_style_set_img_recolor_opa( &style_image, LV_OPA_MAX );
    lv_style_set_img_recolor( &style_image, color_label );
}

void setup_label_style() {
    lv_style_init( &style_label );
    lv_style_set_text_color( &style_label, color_label );
    //    lv_style_set_text_font(&style_label, &rubik_12);
    //    lv_style_set_text_font(&style_label, &rubik_12_subpx);
    lv_style_set_text_font( &style_label, &lv_font_montserrat_12 );
    //    lv_obj_set_style_text_font(ltr_label, &lv_font_montserrat_16, 0);
    lv_style_set_text_align( &style_label, LV_TEXT_ALIGN_CENTER );
}

lv_obj_t *setup_fluid_label( lv_obj_t *parent, int y0 ) {
    static char *label_texts[ ] = { "0", "1/4", "1/2", "3/4", "1" };
    lv_obj_t *label;
    for ( int i = 0; i < 5; i++ ) {
        label = lv_label_create( parent );
        lv_label_set_text_static( label, label_texts[ i ] );
        lv_obj_set_pos( label, y0, SDL_VER_RES - STYLE_BAR_PADDING - STYLE_BAR_OUTLINE -
                                   ( STYLE_BAR_X + STYLE_BAR_PADDING +
                                     i * ( STYLE_BAR_WIDTH - 3 * STYLE_BAR_PADDING ) / 4 -
                                     i ) );
        lv_obj_set_width( label, STYLE_BAR_HEIGHT );
        lv_obj_add_style( label, &style_label, LV_PART_MAIN );
    }
    return label;
}

lv_obj_t *setup_battery_label( lv_obj_t *parent, int y0 ) {
    lv_obj_t *label;
    int start = BATTERY_VOLTAGE_START / 10;
    int end = BATTERY_VOLTAGE_END / 10;
    char label_text[ 4 ];
    int x0 = STYLE_BAR_X + STYLE_BAR_PADDING;
    double multiplier = ( STYLE_BAR_WIDTH - STYLE_BAR_PADDING * 4.0 ) / ( end - start );
    for ( int i = 0; i < end - start + 1; i++ ) {
        label = lv_label_create( parent );
        lv_snprintf( label_text, sizeof( label_text ), "%d", start + i );
        lv_label_set_text( label, label_text );
        lv_obj_set_pos( label, y0,
            SDL_VER_RES - STYLE_BAR_PADDING - STYLE_BAR_OUTLINE - ( (int) ( x0 + i * multiplier ) ) );
        lv_obj_set_width( label, STYLE_BAR_HEIGHT );
        lv_obj_add_style( label, &style_label, LV_PART_MAIN );
    }
    // TODO add line for halves
    //    for( int i = 0; i < end-start+1; i++) {
    //        lv_draw_line();
    //    }
    return label;
}

int obj_get_right( lv_obj_t *obj ) {
    return lv_obj_get_style_x( obj, LV_PART_MAIN ) + lv_obj_get_style_width( obj, LV_PART_MAIN );
}

static void set_water_value( void *bar, int32_t percent ) {
    if ( percent < 0 ) {
        percent = 0;
    } else if ( percent > 100 ) {
        percent = 100;
    }
    int step = percent / 25;
    int value;
    lv_style_t *style;
    if ( step == 0 ) {
        value = STYLE_BAR_EMPTY_STATE;
        style = &style_indic_water_empty;
    } else {
        value = step * 25 + STYLE_BAR_EMPTY_STATE;
        if ( step == 1 ) {
            style = &style_indic_water_deplete;
        } else {
            style = &style_indic_water;
        }
    }
    set_style( bar, style );
    lv_bar_set_value( bar, value, LV_ANIM_ON );
}

static void set_fuel_value( void *bar, int32_t percent ) {
    if ( percent < 0 ) {
        percent = -STYLE_BAR_EMPTY_STATE;
    } else if ( percent > 100 ) {
        percent = 100;
    }
    lv_style_t *style;
    if ( percent <= 3 ) {
        style = &style_indic_water_empty;
    } else if ( percent < 25 ) {
        style = &style_indic_fuel_empty;
    } else {
        style = &style_indic_fuel;
    }
    int value = percent + STYLE_BAR_EMPTY_STATE;
    set_style( bar, style );
    lv_bar_set_value( bar, value, LV_ANIM_ON );
}

static void set_battery_value( void *bar, int32_t value ) {
    if ( value < BATTERY_VOLTAGE_START ) {
        value = BATTERY_VOLTAGE_START;
    } else if ( value > BATTERY_VOLTAGE_END ) {
        value = BATTERY_VOLTAGE_END;
    }
    lv_style_t *style;
    if ( value <= BATTERY_VOLTAGE_WARN_LOW || value >= BATTERY_VOLTAGE_WARN_HIGH ) {
        style = &style_indic_water_empty;
    } else if ( value <= BATTERY_VOLTAGE_WARN_DEPLETE ) {
        style = &style_indic_battery_deplete;
    } else if ( value >= BATTERY_VOLTAGE_CHARGE ) {
        style = &style_indic_battery_charge;
    } else {
        style = &style_indic_battery;
    }
    set_style( bar, style );
    lv_bar_set_value( bar, value, LV_ANIM_ON );
}

void setup_animation_step( lv_obj_t *bar, lv_anim_exec_xcb_t setter ) {
    lv_anim_t anim;
    lv_anim_init( &anim );
    lv_anim_set_exec_cb( &anim, setter );
    lv_anim_set_time( &anim, 60000 );
    lv_anim_set_playback_time( &anim, 60000 );
    lv_anim_set_var( &anim, bar );
    lv_anim_set_values( &anim, 0, 5 );
    lv_anim_set_repeat_count( &anim, LV_ANIM_REPEAT_INFINITE );
    //    lv_anim_set_repeat_count( &anim, 1 );
    lv_anim_start( &anim );
}

void setup_animation_fuel( lv_obj_t *bar, lv_anim_exec_xcb_t setter ) {
    lv_anim_t anim;
    lv_anim_init( &anim );
    lv_anim_set_exec_cb( &anim, setter );
    lv_anim_set_time( &anim, 10000 );
    lv_anim_set_playback_time( &anim, 10000 );
    lv_anim_set_var( &anim, bar );
    lv_anim_set_values( &anim, 0, 100 + STYLE_BAR_EMPTY_STATE );
    lv_anim_set_repeat_count( &anim, LV_ANIM_REPEAT_INFINITE );
    //    lv_anim_set_repeat_count( &anim, 1 );
    lv_anim_start( &anim );
}

void setup_animation_battery( lv_obj_t *bar, lv_anim_exec_xcb_t setter ) {
    lv_anim_t anim;
    lv_anim_init( &anim );
    lv_anim_set_exec_cb( &anim, setter );
    lv_anim_set_time( &anim, 10000 );
    lv_anim_set_playback_time( &anim, 10000 );
    lv_anim_set_var( &anim, bar );
    lv_anim_set_values( &anim, BATTERY_VOLTAGE_START, BATTERY_VOLTAGE_END );
    lv_anim_set_repeat_count( &anim, LV_ANIM_REPEAT_INFINITE );
    //    lv_anim_set_repeat_count( &anim, 1);
    lv_anim_start( &anim );
}

#define SPACE_BETWEEN_BARS  (STYLE_BAR_HEIGHT + 2 * STYLE_BAR_OUTLINE)

void display_meter_main() {
    setup_screen();

    setup_bar_background_style();
    setup_battery_style();
    setup_image_style();
    setup_label_style();

    lv_obj_t *screen = lv_scr_act();
    lv_obj_t *prev;
    prev = bar_water = setup_water_bar( screen, 25 );
    setup_fluid_label( screen, obj_get_right( prev ) + STYLE_BAR_OUTLINE );
    prev = bar_fuel = setup_fuel_bar( screen, obj_get_right( prev ) + SPACE_BETWEEN_BARS );
    prev = bar_battery[ 0 ] = setup_battery_bar( screen,
               obj_get_right( prev ) + STYLE_BAR_HEIGHT + SPACE_BETWEEN_BARS, 0 );
    setup_battery_label( screen, obj_get_right( prev ) + STYLE_BAR_OUTLINE );
    prev = bar_battery[ 1 ] = setup_battery_bar( screen,
               obj_get_right( prev ) + SPACE_BETWEEN_BARS, 1 );
    setup_battery_label( screen, obj_get_right( prev ) + STYLE_BAR_OUTLINE );
    bar_battery[ 2 ] = setup_battery_bar( screen,
        obj_get_right( prev ) + SPACE_BETWEEN_BARS, 2 );

#if USE_ANIMATION
    setup_animation_step( bar_water, set_water_value );
    setup_animation_fuel( bar_fuel, set_fuel_value );
    for ( int i = 0; i < 3; i++ ) {
        setup_animation_battery( bar_battery[ i ], set_battery_value );
    }
#else
    set_water_value( bar_water, 0 );
    set_fuel_value( bar_fuel, 0 );
    for ( int i = 0; i < sizeof( bar_battery ) / sizeof( bar_battery[ 0 ] ); ++i ) {
        set_battery_value( bar_battery[ i ], 0 );
    }
#endif
    initialized = true;
}

void display_meter_set_value( display_type_t type, int instance, int value ) {
    //    ESP_LOGI( LOG, "type %d/%s inst %d value %d", type, display_type_names[ type ], instance, value );
    if ( !initialized ) {
        return;
    }
    if ( !display_start_task() ) {
        return;
    }
    switch ( type ) {
        case FUEL:
            set_fuel_value( bar_fuel, value );
            break;
        case VOLTAGE:
            if ( instance >= 0 && instance < sizeof( bar_battery ) / sizeof( bar_battery[ 0 ] ) ) {
                set_battery_value( bar_battery[ instance ], value );
            } else {
                ESP_LOGW( LOG, "Unhandled battery instance %d", instance );
            }
            break;
        case WATER:
            set_water_value( bar_water, value );
            break;
        case TEMP:
            break;
        default:
            ESP_LOGW( LOG, "Unhandled display type %d", type );
            break;
    }
    display_end_task();
}

const char *display_type_names[ ] = {
    "FUEL",
    "VOLTAGE",
    "WATER",
    "TEMP"
};
