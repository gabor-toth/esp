//
// Created by tothg on 2023.01.11..
//

#include "hajo_can.h"

#include "driver/twai.h"

/*
static QueueHandle_t dummy_timer_event_queue = NULL;

_Noreturn static void dummy_timer_event( void *arg ) {
    for ( ;; ) {
        uint32_t dummy;
        if ( xQueueReceive( dummy_timer_event_queue, &dummy, portMAX_DELAY )) {
        }
    }
}

static void dummy_timer_callback( TimerHandle_t timer ) {
    uint32_t dummy = 0;
    xQueueSend( dummy_timer_event_queue, &dummy, 0 );
}

void dummy_timer_start() {
    dummy_timer_event_queue = xQueueCreate( 10, sizeof( uint32_t ));
    xTaskCreate( dummy_timer_event, "dummy", 2048, NULL, 10, NULL);

    TimerHandle_t timer = xTimerCreate(
            "dummy",
            pdMS_TO_TICKS( 30 * 1000 ),
            1,
            NULL,
            dummy_timer_callback );
    xTimerStart( timer, portMAX_DELAY );
}
 */

void can_main() {
    //Initialize configuration structures using macro initializers
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT( GPIO_NUM_4, GPIO_NUM_5, TWAI_MODE_NORMAL );
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    //Install TWAI driver
    if ( twai_driver_install( &g_config, &t_config, &f_config ) == ESP_OK ) {
        printf( "Driver installed\n" );
    } else {
        printf( "Failed to install driver\n" );
        return;
    }

    //Start TWAI driver
    if ( twai_start() == ESP_OK ) {
        printf( "Driver started\n" );
    } else {
        printf( "Failed to start driver\n" );
        return;
    }

    //esp_err_t twai_receive(twai_message_t *message, TickType_t ticks_to_wait)
}

