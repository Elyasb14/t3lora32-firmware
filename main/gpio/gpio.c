#include <driver/gpio.h>
#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/task.h>
#include <stdatomic.h>
#include <stdbool.h>

#define GPIO_LED 25
#define DIO0_PIN 26
#define LORA_EVENT_DIO0 (1U << 1)

static void IRAM_ATTR dio0_isr_handler(void *arg) {
    TaskHandle_t lora_task_handle = arg;
    BaseType_t higher_priority_task_woken = pdFALSE;

    xTaskNotifyFromISR(
        lora_task_handle,
        LORA_EVENT_DIO0,
        eSetBits,
        &higher_priority_task_woken);

    portYIELD_FROM_ISR(higher_priority_task_woken);
}

void gpio_init_interrupt(TaskHandle_t task_handle) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << DIO0_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.intr_type = GPIO_INTR_POSEDGE; // Trigger on rising edge

    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(DIO0_PIN, dio0_isr_handler, task_handle);
}

void gpio_blink_led(void) {
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);

    gpio_set_level(GPIO_LED, 1);
    vTaskDelay(portTICK_PERIOD_MS);
    gpio_set_level(GPIO_LED, 0);
}
