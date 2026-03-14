#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include <esp_attr.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>

#define GPIO_LED 25
#define DIO0_PIN 26

static volatile atomic_bool dio0_triggered = ATOMIC_VAR_INIT(false);

static void IRAM_ATTR dio0_isr_handler(void *arg) {
    atomic_store(&dio0_triggered, true);
}

void gpio_init_interrupt(void) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << DIO0_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.intr_type = GPIO_INTR_POSEDGE; // Trigger on rising edge

    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(DIO0_PIN, dio0_isr_handler, NULL);
}

bool gpio_check_dio0_and_clear(void) {
    bool triggered = atomic_exchange(&dio0_triggered, false);
    return triggered;
}

void gpio_blink_led(void) {
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);

    gpio_set_level(GPIO_LED, 1);
    vTaskDelay(portTICK_PERIOD_MS);
    gpio_set_level(GPIO_LED, 0);
}
