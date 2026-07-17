#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "gpio/gpio.h"
#include "i2c/i2c.h"
#include "lora/lora.h"
#include "oled/oled.h"
#include <driver/gpio.h>
#include <driver/i2c_types.h>
#include <driver/spi_master.h>
#include <driver/uart.h>
#include <freertos/task.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char *buf;
    spi_device_handle_t handle;
} TTYArgs;

void tty_task(void *arg) {
    TTYArgs *tty_args = arg;

    uint8_t data[256];

    while (1) {
        int len = uart_read_bytes(
            UART_NUM_0,
            data,
            256,
            pdMS_TO_TICKS(10));

        if (len > 0) {
            printf("GOT PACKET: ");
            fwrite(data, 1, len, stdout);
            printf("\n");

            lora_send_packet(
                tty_args->handle,
                data,
                len);
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

    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
}

void app_main() {
    i2c_master_dev_handle_t i2c_handle = i2c_init();
    oled_init(i2c_handle);

    spi_device_handle_t handle = lora_init();

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
    init_uart();

    lora_set_dio0_mapping(handle, false);

    lora_set_mode_rx_continuous(handle);

    uint8_t rx_buf[256];
    char tty_buf[256];

    TTYArgs tty_args = {.buf = tty_buf, .handle = handle};
    xTaskCreate(tty_task, "tty_task", 4096, &tty_args, 1, NULL);

    while (1) {

        if (gpio_check_dio0_and_clear()) {
            uint8_t flags = lora_get_irq_flags(handle);

            if (flags & RFLR_IRQFLAGS_RXDONE) {
                gpio_blink_led();
                gpio_blink_led();

                uint16_t rx_len = (uint16_t)lora_get_rx_payload_length(handle);
                if (rx_len > sizeof(rx_buf)) {
                    rx_len = sizeof(rx_buf);
                }

                lora_read_fifo_payload(handle, rx_buf,
                                       (uint8_t)rx_len);

                printf("RCVD PKT:");
                for (int i = 0; i < rx_len; i++) {
                    printf("%c", rx_buf[i]);
                }

                lora_clear_irq_flags(handle, RFLR_IRQFLAGS_RXDONE);
            }

            if (flags & RFLR_IRQFLAGS_TXDONE) {

                gpio_blink_led();

                lora_clear_irq_flags(handle, RFLR_IRQFLAGS_TXDONE);

                lora_set_dio0_mapping(handle, false);
                lora_set_mode_rx_continuous(handle);
            }

            if (flags & RFLR_IRQFLAGS_PAYLOADCRCERROR) {
                lora_clear_irq_flags(handle, RFLR_IRQFLAGS_PAYLOADCRCERROR);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
