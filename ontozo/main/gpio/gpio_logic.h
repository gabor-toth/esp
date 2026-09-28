#ifndef ONTOZO_GPIO_LOGIC_H
#define ONTOZO_GPIO_LOGIC_H

#include "stdbool.h"

// outputs

#define PUMPS_CLASS 0
#define ZONES_CLASS 1

// inputs

#define LEVELS_CLASS 0
#define BUTTONS_CLASS 1

extern void gpio_logic_init();

extern void gpio_pump_main( bool on );

extern int classLevels;
extern int classButtons;

#endif //ONTOZO_GPIO_LOGIC_H
