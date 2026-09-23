#ifndef ONTOZO_POWER_LED_H
#define ONTOZO_POWER_LED_H

#define GPIO_NUM_LED_POWER      GPIO_NUM_15

extern void power_led_main();

#define POWER_LED_MODE_STARTUP      0
#define POWER_LED_MODE_WIFI_SCAN    1
#define POWER_LED_MODE_RUNNING      2

extern void power_led_set_mode(int mode);

#endif //ONTOZO_POWER_LED_H
