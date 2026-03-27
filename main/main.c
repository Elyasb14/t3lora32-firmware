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

#define UART_PORT UART_NUM_0

void app_main() {
    printf("\n");

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
    oled_draw_string(i2c_handle, "mesh repeater", 0, 2);
    oled_draw_string(i2c_handle, freq_buffer, 0, 3);
    oled_draw_string(i2c_handle, bw_buffer, 0, 4);
    oled_draw_string(i2c_handle, tx_buffer, 0, 5);

    gpio_init_interrupt();
    printf("GPIO Interrupt Initialized\n");

    lora_set_dio0_mapping(handle, false);

    lora_set_mode_rx_continuous(handle);
    printf("Entering RX Continuous Mode...\n");

    uint8_t rx_buf[256];

    while (1) {
        if (gpio_check_dio0_and_clear()) {
            uint8_t flags = lora_get_irq_flags(handle);

            if (flags & RFLR_IRQFLAGS_RXDONE) {
                gpio_blink_led();

                uint8_t rx_len = lora_get_rx_payload_length(handle);
                uint16_t bytes_to_read = rx_len;
                if (bytes_to_read > sizeof(rx_buf)) {
                    bytes_to_read = sizeof(rx_buf);
                }

                lora_read_fifo_payload(handle, rx_buf,
                                       (uint8_t)bytes_to_read);

                printf("Received %d bytes (Hex): ", bytes_to_read);
                for (int i = 0; i < bytes_to_read; i++) {
                    printf("%02X ", rx_buf[i]);
                }
                printf("\n");

                printf("Received String: '");
                for (int i = 0; i < bytes_to_read; i++) {
                    if (rx_buf[i] >= 32 && rx_buf[i] <= 126) {
                        printf("%c", rx_buf[i]);
                    } else {
                        printf(".");
                    }
                }
                printf("'\n");

                lora_clear_irq_flags(handle, RFLR_IRQFLAGS_RXDONE);
            }

            if (flags & RFLR_IRQFLAGS_TXDONE) {
                gpio_blink_led();
                printf("Packet sent successfully (TX Done Interrupt)\n");

                lora_clear_irq_flags(handle, RFLR_IRQFLAGS_TXDONE);

                lora_set_dio0_mapping(handle, false);
                lora_set_mode_rx_continuous(handle);
                printf("Returned to RX Continuous Mode\n");
            }

            if (flags & RFLR_IRQFLAGS_PAYLOADCRCERROR) {
                printf("CRC Error in received packet\n");
                lora_clear_irq_flags(handle, RFLR_IRQFLAGS_PAYLOADCRCERROR);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
