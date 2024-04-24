#include "display_main.h"
#include "display_meter.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"
//#include "esp_lcd_backlight.h"
#include "lvgl/lvgl.h"
#include <stdlib.h>

/*********************
 *      DEFINES
 *********************/

#define LV_TICK_PERIOD_MS 10

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void guiTask( void *pvParameter );

static void lv_tick_task( void *arg );

static void backlight_callback( TimerHandle_t timer );

/**********************
 *  STATIC VARIABLES
 **********************/

static const char *LOG = "display";

static SemaphoreHandle_t xGuiSemaphore;
//static disp_backlight_config_t *backlight_handler;
static int backlight_off_interval = 5;
static TimerHandle_t backlight_timer;
static bool is_display_on;
static lv_indev_t *indev;
static lv_indev_state_t indev_previous_state;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void display_main() {
    /* If you want to use a task to create the graphic, you NEED to create a Pinned task
     * Otherwise there can be problem such as memory corruption and so on.
     * NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 */
    xTaskCreatePinnedToCore( guiTask, "gui", 4096 * 2, NULL, 0, NULL, 0 );
    
    backlight_timer = xTimerCreate(
            "backlight",
            pdMS_TO_TICKS( backlight_off_interval * 1000 ),
            0,
            NULL,
            backlight_callback );
}

bool display_start_task() {
    return pdTRUE == xSemaphoreTake( xGuiSemaphore, portMAX_DELAY );
}

void display_end_task() {
    xSemaphoreGive( xGuiSemaphore );
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void backlight_on() {
//    disp_backlight_set( backlight_handler, 100 );
    is_display_on = true;
    xTimerStart( backlight_timer, portMAX_DELAY );
    ESP_LOGI( LOG, "backlight_on" );
}

static void backlight_off() {
//    disp_backlight_set( backlight_handler, 0 );
    is_display_on = false;
    xTimerStop( backlight_timer, portMAX_DELAY );
    ESP_LOGI( LOG, "backlight_off" );
}

static void indev_read( lv_indev_t *indev, lv_indev_data_t *data ) {
//    touch_driver_read( drv, data );
    
    lv_indev_state_t current_state = lv_indev_get_state( indev );
    if ( indev_previous_state != current_state ) {
        ESP_LOGI( LOG, "current_state %d, previous_state %d, display_on %d",
                  current_state, indev_previous_state, is_display_on );
        if ( current_state == LV_INDEV_STATE_PRESSED && !is_display_on ) {
            backlight_on();
        } else if ( current_state == LV_INDEV_STATE_RELEASED && is_display_on ) {
            xTimerReset( backlight_timer, portMAX_DELAY );
        }
        indev_previous_state = current_state;
    }
}

static void guiTask( void *pvParameter ) {
    xGuiSemaphore = xSemaphoreCreateMutex();
    
    /*Initialize LVGL*/
    lv_init();
    
    /* Initialize SPI or I2C bus used by the drivers */
//    backlight_handler = lvgl_driver_init();
//    backlight_on();
    
    uint32_t size_in_px = CONFIG_LV_DRAW_LAYER_SIMPLE_BUF_SIZE;
    uint32_t size_in_bytes = size_in_px * sizeof( lv_color_t );
    lv_color_t *buf1 = heap_caps_malloc( size_in_bytes, MALLOC_CAP_DMA );
    assert( buf1 != NULL );
    lv_color_t *buf2 = heap_caps_malloc( size_in_bytes, MALLOC_CAP_DMA );
    assert( buf2 != NULL );
    
    lv_display_t *disp = lv_display_create( 240, 320 );
//    lv_display_set_flush_cb( disp, disp_driver_flush );
    lv_display_set_buffers( disp, buf1, buf2, size_in_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL );
    
    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &lv_tick_task,
            .name = "lvgl_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK( esp_timer_create( &periodic_timer_args, &periodic_timer ) );
    ESP_ERROR_CHECK( esp_timer_start_periodic( periodic_timer, LV_TICK_PERIOD_MS * 1000 ) );
    
    indev = lv_indev_create();
    lv_indev_set_type( indev, LV_INDEV_TYPE_POINTER );
    lv_indev_set_read_cb( indev, indev_read );
    
    display_meter_main();
    
    while ( 1 ) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay( pdMS_TO_TICKS( 10 ) );
        
        /* Try to take the semaphore, call lvgl related function on success */
        if ( display_start_task() ) {
            lv_timer_handler();
            display_end_task();
        }
    }
}

void display_set_value( display_type_t type, int instance, int value ) {
    display_meter_set_value( type, instance, value );
}

static void lv_tick_task( void *arg ) {
    (void) arg;
    
    lv_tick_inc( LV_TICK_PERIOD_MS );
}

static void backlight_callback( TimerHandle_t timer ) {
    (void) timer;
    
    backlight_off();
}

