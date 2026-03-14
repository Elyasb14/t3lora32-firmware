#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>

void gpio_blink_led(void);
void gpio_init_interrupt(void);
bool gpio_check_dio0_and_clear(void);

#endif // GPIO_H
