#include "display_main.h"
#include "display_meter.h"
#include "esp_lcd_backlight.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
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

/**********************
 *  STATIC VARIABLES
 **********************/

static SemaphoreHandle_t xGuiSemaphore;
static disp_backlight_config_t *backlight_handler;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void display_main() {
    /* If you want to use a task to create the graphic, you NEED to create a Pinned task
     * Otherwise there can be problem such as memory corruption and so on.
     * NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 */
    xTaskCreatePinnedToCore( guiTask, "gui", 4096 * 2, NULL, 0, NULL, 0 );
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

static void guiTask( void *pvParameter ) {
    xGuiSemaphore = xSemaphoreCreateMutex();

    /*Initialize LVGL*/
    lv_init();

    /* Initialize SPI or I2C bus used by the drivers */
    backlight_handler = lvgl_driver_init();
    disp_backlight_set( backlight_handler, 100 );

    lv_color_t *buf1 = heap_caps_malloc( DISP_BUF_SIZE * sizeof( lv_color_t ), MALLOC_CAP_DMA);
    assert( buf1 != NULL );
    lv_color_t *buf2 = heap_caps_malloc( DISP_BUF_SIZE * sizeof( lv_color_t ), MALLOC_CAP_DMA);
    assert( buf2 != NULL );

    static lv_disp_draw_buf_t disp_buf;

    uint32_t size_in_px = DISP_BUF_SIZE;

    lv_disp_draw_buf_init( &disp_buf, buf1, buf2, size_in_px );

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register( &disp_drv );

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &lv_tick_task,
            .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK( esp_timer_create( &periodic_timer_args, &periodic_timer ));
    ESP_ERROR_CHECK( esp_timer_start_periodic( periodic_timer, LV_TICK_PERIOD_MS * 1000 ));

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register( &indev_drv );

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
