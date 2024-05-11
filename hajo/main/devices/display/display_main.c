#include "display_main.h"
#include "display_meter.h"
#include "esp_lcd_backlight.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "lvgl_esp32_drivers/lvgl_helpers.h"
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

static void timer_callback_backlight( TimerHandle_t timer );

/**********************
 *  STATIC VARIABLES
 **********************/

static const char *LOG = "display";

static SemaphoreHandle_t xGuiSemaphore;
static disp_backlight_config_t *backlight_handler;
static int backlight_off_interval = 30;
static TimerHandle_t backlight_timer;
static bool is_display_on;
static volatile bool turn_off_display;
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
            timer_callback_backlight );
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
    disp_backlight_set( backlight_handler, 100 );
    is_display_on = true;
    xTimerStart( backlight_timer, portMAX_DELAY );
    ESP_LOGI( LOG, "backlight_on" );
}

static void backlight_off() {
    disp_backlight_set( backlight_handler, 0 );
    is_display_on = false;
    xTimerStop( backlight_timer, portMAX_DELAY );
    ESP_LOGI( LOG, "backlight_off" );
}

static void indev_read( lv_indev_drv_t *drv, lv_indev_data_t *data ) {
    touch_driver_read( drv, data );

    lv_indev_state_t current_state = indev->proc.state;
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
    backlight_handler = lvgl_driver_init();
    backlight_on();

    uint32_t size_in_px = DISP_BUF_SIZE;

    lv_color_t *buf1 = heap_caps_malloc( size_in_px * sizeof( lv_color_t ), MALLOC_CAP_DMA );
    assert( buf1 != NULL );
//    lv_color_t *buf2 = NULL;
    lv_color_t *buf2 = heap_caps_malloc( size_in_px * sizeof( lv_color_t ), MALLOC_CAP_DMA );
    assert( buf2 != NULL );

    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init( &disp_buf, buf1, buf2, size_in_px );

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
#if CONFIG_LV_DISPLAY_ORIENTATION_PORTRAIT
    disp_drv.rotated = 1;
#elif CONFIG_LV_DISPLAY_ORIENTATION_PORTRAIT_INVERTED
    disp_drv.rotated = 3;
#elif CONFIG_LV_DISPLAY_ORIENTATION_LANDSCAPE
    disp_drv.rotated = 0;
#elif CONFIG_LV_DISPLAY_ORIENTATION_LANDSCAPE_INVERTED
    disp_drv.rotated = 2;
#else
#   error Unknown screen orientation
#endif
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &disp_buf;
    ESP_LOGI( LOG, "drv horiz %d vert %d", disp_drv.hor_res, disp_drv.ver_res );
    lv_disp_t *disp = lv_disp_drv_register( &disp_drv );

    lv_disp_t *disp_def = lv_disp_get_default();
    ESP_LOGI( LOG, "own %p def %p", disp, disp_def );
    ESP_LOGI( LOG, "own horiz %d vert %d", lv_disp_get_hor_res( disp ), lv_disp_get_ver_res( disp ) );
    ESP_LOGI( LOG, "def horiz %d vert %d", lv_disp_get_hor_res( disp_def ), lv_disp_get_ver_res( disp_def ) );

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &lv_tick_task,
            .name = "lvgl_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK( esp_timer_create( &periodic_timer_args, &periodic_timer ) );
    ESP_ERROR_CHECK( esp_timer_start_periodic( periodic_timer, LV_TICK_PERIOD_MS * 1000 ) );

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.read_cb = indev_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev = lv_indev_drv_register( &indev_drv );

    ESP_LOGI( LOG, "before display_meter_main" );
    display_meter_main();
    ESP_LOGI( LOG, "after display_meter_main" );

    uint32_t next_log_time = 0;
    while ( 1 ) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay( pdMS_TO_TICKS( 10 ) );

        if ( turn_off_display ) {
            turn_off_display = false;
            backlight_off();
        }

        /* Try to take the semaphore, call lvgl related function on success */
        if ( display_start_task() ) {
            lv_task_handler();
            display_end_task();
        }

        uint32_t current_time = lv_tick_get();
        if ( current_time > next_log_time ) {
            next_log_time = current_time + 1000;
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

static void timer_callback_backlight( TimerHandle_t timer ) {
    (void) timer;

    turn_off_display = true;
}
