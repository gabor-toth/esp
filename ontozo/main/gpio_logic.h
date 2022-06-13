#ifndef ONTOZO_GPIO_LOGIC_H
#define ONTOZO_GPIO_LOGIC_H

// outputs

#define PUMPS 0
#define ZONES 1

// inputs

#define LEVELS 0
#define BUTTONS 1

extern void gpio_logic_init();

extern void gpio_pump_main( bool on );

#endif //ONTOZO_GPIO_LOGIC_H
