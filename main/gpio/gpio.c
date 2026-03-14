#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define GPIO_LED 25

void gpio_blink_led(void) {
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);

    gpio_set_level(GPIO_LED, 1);
    vTaskDelay(portTICK_PERIOD_MS);
    gpio_set_level(GPIO_LED, 0);
}
