#ifndef ONTOZO_GPIO_LOGIC_H
#define ONTOZO_GPIO_LOGIC_H

#include "stdbool.h"

// outputs

#define PIN_CLASS_OUT_PUMPS 0
#define PIN_CLASS_OUT_ZONES 1
#define PIN_CLASS_OUT_BUTTONS 2

// inputs

#define PIN_CLASS_IN_LEVELS 0
#define PIN_CLASS_IN_BUTTONS 1

extern void gpio_logic_init();

extern void gpio_pump_main( bool on );

extern int classLevels;
extern int classButtons;

#endif //ONTOZO_GPIO_LOGIC_H
