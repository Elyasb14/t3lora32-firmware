#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "gpio/gpio.h"
#include "i2c/i2c.h"
#include "lora/lora.h"
#include "oled/oled.h"
#include "portmacro.h"
#include <driver/gpio.h>
#include <driver/i2c_types.h>
#include <driver/spi_master.h>
#include <driver/uart.h>
#include <freertos/task.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define configSUPPORT_STATIC_ALLOCATION 1
#define LORA_QUEUE_LENGTH 10
#define LORA_ITEM_SIZE sizeof(LoraQueueItem)

typedef struct {
    uint8_t *data;
    uint8_t len;
} LoraQueueItem;

typedef struct {
    char *buf;
    QueueHandle_t lora_queue_handle;
    TaskHandle_t lora_task_handle;
} UARTArgs;

typedef struct {
    uint8_t *buf;
    spi_device_handle_t handle;
    QueueHandle_t lora_queue_handle;
} LoraArgs;

void uart_task(void *arg) {
    UARTArgs *uart_args = arg;

    uint8_t data[256];

    while (1) {

        int len = uart_read_bytes(
            UART_NUM_0,
            data,
            256,
            pdMS_TO_TICKS(10));

        LoraQueueItem lora_queue_item = {.len = len, .data = data};

        if (len > 0) {
            xQueueSend(uart_args->lora_queue_handle, (void *)&lora_queue_item, portMAX_DELAY);
        }
    }
}
void init_uart() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_driver_install(UART_NUM_0, 1024, 1024, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
}

static TaskHandle_t lora_task_handle;
QueueHandle_t lora_tx_queue;
static UARTArgs uart_args;
static LoraArgs lora_args;

void lora_task(void *args) {
    LoraArgs *lora_args = args;

    LoraQueueItem lora_queue_item;

    while (1) {

        if (xQueueReceive(lora_args->lora_queue_handle, &(lora_queue_item), portMAX_DELAY)) {
            lora_send_packet(lora_args->handle, lora_queue_item.data, lora_queue_item.len);
        }

        if (gpio_check_dio0_and_clear()) {
            uint8_t flags = lora_get_irq_flags(lora_args->handle);

            if (flags & RFLR_IRQFLAGS_RXDONE) {
                gpio_blink_led();
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_blink_led();

                uint16_t rx_len = (uint16_t)lora_get_rx_payload_length(lora_args->handle);
                if (rx_len > sizeof(lora_args->buf)) {
                    rx_len = sizeof(lora_args->buf);
                }

                lora_read_fifo_payload(lora_args->handle, lora_args->buf,
                                       (uint8_t)rx_len);

                int written = uart_write_bytes(UART_NUM_0, (const char *)lora_args->buf, rx_len);
                printf("wrote %d/%d bytes\n", written, rx_len);

                lora_clear_irq_flags(lora_args->handle, RFLR_IRQFLAGS_RXDONE);
            }

            if (flags & RFLR_IRQFLAGS_TXDONE) {

                gpio_blink_led();

                lora_clear_irq_flags(lora_args->handle, RFLR_IRQFLAGS_TXDONE);

                lora_set_dio0_mapping(lora_args->handle, false);
                lora_set_mode_rx_continuous(lora_args->handle);
            }

            if (flags & RFLR_IRQFLAGS_PAYLOADCRCERROR) {
                lora_clear_irq_flags(lora_args->handle, RFLR_IRQFLAGS_PAYLOADCRCERROR);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

QueueHandle_t create_lora_queue() {

    static uint8_t lora_queue_storage_area[LORA_QUEUE_LENGTH * LORA_ITEM_SIZE];
    static StaticQueue_t lora_tx_queue;

    QueueHandle_t lora_queue_handle;

    lora_queue_handle = xQueueCreateStatic(LORA_QUEUE_LENGTH, LORA_ITEM_SIZE, lora_queue_storage_area, &lora_tx_queue);
    return lora_queue_handle;
}

void app_main() {
    i2c_master_dev_handle_t i2c_handle = i2c_init();
    oled_init(i2c_handle);

    spi_device_handle_t handle = lora_init();

    init_uart();

    lora_set_tx_power(handle, 1);
    lora_set_frequency(handle, 903000000);
    lora_set_bandwidth(handle, LORA_BW_125_KHZ);

    char bw_buffer[32];
    char tx_buffer[32];
    char freq_buffer[32];

    snprintf(bw_buffer, sizeof(bw_buffer), "BW: %d kHz",
             lora_get_bandwidth(handle));
    snprintf(tx_buffer, sizeof(tx_buffer), "TX: %d dBm",
             lora_get_tx_power(handle));
    snprintf(freq_buffer, sizeof(freq_buffer), "Freq: %.2f MHz",
             lora_get_freq(handle));

    oled_clear_display(i2c_handle);
    oled_draw_string(i2c_handle, freq_buffer, 0, 3);
    oled_draw_string(i2c_handle, bw_buffer, 0, 4);
    oled_draw_string(i2c_handle, tx_buffer, 0, 5);

    gpio_init_interrupt();

    lora_set_dio0_mapping(handle, false);

    lora_set_mode_rx_continuous(handle);

    static uint8_t rx_buf[LORA_QUEUE_LENGTH * LORA_ITEM_SIZE];
    static char uart_buf[256];

    QueueHandle_t lora_queue_handle = create_lora_queue();
    TaskHandle_t lora_task_handle;

    uart_args = (UARTArgs){.buf = uart_buf, .lora_queue_handle = lora_queue_handle, .lora_task_handle = lora_task_handle};
    xTaskCreate(uart_task, "uart_read_task", 4096, &uart_args, 1, NULL);

    lora_args = (LoraArgs){.handle = handle, .buf = rx_buf, .lora_queue_handle = lora_queue_handle};
    xTaskCreate(lora_task, "lora_task", 4096, &lora_args, 1, &lora_task_handle);
}
