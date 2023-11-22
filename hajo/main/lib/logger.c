#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logger.h"

char *currentTaskName() {
    return pcTaskGetName( xTaskGetCurrentTaskHandle());
}
