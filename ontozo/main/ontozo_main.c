#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "lib/gpio_define.h"
#include "test_util.h"
#include "lib/rest_main.h"

void app_main( void ) {
    gpio_init();
    rest_init();
//    test_init();
}
