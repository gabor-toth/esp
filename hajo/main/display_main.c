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

static void backlight_callback( TimerHandle_t timer );

/**********************
 *  STATIC VARIABLES
 **********************/

static const char *LOG = "display";

static SemaphoreHandle_t xGuiSemaphore;
static disp_backlight_config_t *backlight_handler;
static int backlight_off_interval = 30;
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

//    ESP_LOGI( LOG, "indev_read state %d pn %d", indev->proc.state, is_display_on );
    lv_indev_state_t current_state = indev->proc.state;
    if ( indev_previous_state != current_state ) {
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

    lv_color_t *buf1 = heap_caps_malloc( size_in_px * sizeof( lv_color_t ), MALLOC_CAP_DMA);
    assert( buf1 != NULL );
    lv_color_t *buf2 = heap_caps_malloc( size_in_px * sizeof( lv_color_t ), MALLOC_CAP_DMA);
    assert( buf2 != NULL );

    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init( &disp_buf, buf1, buf2, size_in_px );

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register( &disp_drv );

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &lv_tick_task,
            .name = "lvgl_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK( esp_timer_create( &periodic_timer_args, &periodic_timer ));
    ESP_ERROR_CHECK( esp_timer_start_periodic( periodic_timer, LV_TICK_PERIOD_MS * 1000 ));

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.read_cb = indev_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev = lv_indev_drv_register( &indev_drv );

    display_meter_main();

    while ( 1 ) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS( 10 ));

        /* Try to take the semaphore, call lvgl related function on success */
        if ( display_start_task()) {
            lv_task_handler();
            display_end_task();
        }
    }
}

static void lv_tick_task( void *arg ) {
    (void) arg;

    lv_tick_inc( LV_TICK_PERIOD_MS );
}

static void backlight_callback( TimerHandle_t timer ) {
    (void) timer;

    backlight_off();
}

