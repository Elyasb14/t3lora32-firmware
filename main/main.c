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

#include "gpio/gpio.c"
#include "i2c/i2c.c"
#include "lora/lora.c"
#include "oled/oled.c"

#define TX_TRIGGER_CHAR 't'
#define UART_PORT UART_NUM_0

// Forward declarations
void send_packet_manual(spi_device_handle_t handle);

void app_main() {
    // Initialize UART for manual trigger
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_PORT, &uart_config);
    uart_driver_install(UART_PORT, 256, 0, 0, NULL, 0);

    spi_device_handle_t handle = lora_init();

    lora_set_tx_power(handle, 1);
    lora_set_frequency(handle, 903000000);

    printf("TX Power: %d dBm\n", lora_get_tx_power(handle));
    printf("Frequency: %.2f MHz\n", lora_get_freq(handle));

    gpio_init_interrupt();
    printf("GPIO Interrupt Initialized\n");

    lora_set_dio0_mapping(handle, false);

    lora_set_mode_rx_continuous(handle);
    printf("Entering RX Continuous Mode...\n");

    uint8_t rx_buf[256];

    while (1) {
        uint8_t ch;
        if (uart_read_bytes(UART_PORT, &ch, 1, 0) > 0) {
            if (ch >= 32 && ch <= 126) {
                printf("Manual TX Trigger received (char: %c)\n", ch);
                send_packet_manual(handle);
            }
        }

        if (gpio_check_dio0_and_clear()) {
            uint8_t flags = lora_get_irq_flags(handle);

            if (flags & RFLR_IRQFLAGS_RXDONE) {
                gpio_blink_led();

                uint8_t rx_len = lora_get_rx_payload_length(handle);
                uint16_t bytes_to_read = rx_len;
                if (bytes_to_read > sizeof(rx_buf)) {
                    bytes_to_read = sizeof(rx_buf);
                }

                lora_read_fifo_payload(handle, rx_buf, (uint8_t)bytes_to_read);

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

                lora_set_dio0_mapping(handle, false); // Map DIO0 to RxDone
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

void send_packet_manual(spi_device_handle_t handle) {
    char *data = "hello";
    printf("Transmitting: '%s'\n", data);

    lora_set_dio0_mapping(handle, true);

    lora_set_mode_standby(handle);
    lora_write_reg(handle, REG_LR_FIFOTXBASEADDR, 0x00);
    lora_write_reg(handle, REG_LR_FIFOADDRPTR, 0x00);
    lora_write_reg(handle, REG_LR_PAYLOADLENGTH, strlen(data));

    for (int i = 0; i < strlen(data); i++) {
        lora_write_reg(handle, REG_LR_FIFO, data[i]);
    }

    lora_set_mode_tx(handle);
    printf("TX Mode Started, waiting for interrupt...\n");
}
