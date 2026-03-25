#include "esp_err.h"
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
#define OLED_TASK_INTERVAL_MS 10
#define OLED_SHIFT_INTERVAL_TICKS 500

static void oled_task(void *arg) {
    oled_state_t *state = (oled_state_t *)arg;

    while (1) {
        if (state->dirty) {
            oled_redraw(state);
        }

        state->shift_timer++;
        if (state->shift_timer >= OLED_SHIFT_INTERVAL_TICKS) {
            state->shift_timer = 0;

            if (state->shift_offset == 0) {
                state->shift_offset = -2;
            } else if (state->shift_offset == -2) {
                state->shift_offset = 2;
            } else {
                state->shift_offset = 0;
            }

            state->dirty = true;
        }

        vTaskDelay(pdMS_TO_TICKS(OLED_TASK_INTERVAL_MS));
    }
}

void app_main() {
    printf("\n");
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

    i2c_master_dev_handle_t i2c_handle = i2c_init();
    oled_state_t oled = oled_init(i2c_handle);

    spi_device_handle_t handle = NULL;
    if (lora_init(&handle) != ESP_OK) {
        printf("LoRa init failed\n");
        return;
    }

    ESP_ERROR_CHECK(lora_set_tx_power(handle, 1));
    ESP_ERROR_CHECK(lora_set_frequency(handle, 903000000));
    ESP_ERROR_CHECK(lora_set_bandwidth(handle, LORA_BW_125_KHZ));

    char bw_buffer[32];
    char tx_buffer[32];
    char freq_buffer[32];

    lora_bandwidth_t bw;
    uint8_t tx_dbm;
    float freq_mhz;
    ESP_ERROR_CHECK(lora_get_bandwidth(handle, &bw));
    ESP_ERROR_CHECK(lora_get_tx_power(handle, &tx_dbm));
    ESP_ERROR_CHECK(lora_get_freq(handle, &freq_mhz));

    snprintf(bw_buffer, sizeof(bw_buffer), "BW: %d kHz", bw);
    snprintf(tx_buffer, sizeof(tx_buffer), "TX: %d dBm", tx_dbm);
    snprintf(freq_buffer, sizeof(freq_buffer), "Freq: %.2f MHz", freq_mhz);

    oled_clear_pages(&oled);
    oled_set_text(&oled, 2, "mesh repeater");
    oled_set_text(&oled, 3, freq_buffer);
    oled_set_text(&oled, 4, bw_buffer);
    oled_set_text(&oled, 5, tx_buffer);

    xTaskCreatePinnedToCore(oled_task, "oled_task", 3072, &oled,
                            configMAX_PRIORITIES - 3, NULL, 1);

    gpio_init_interrupt();
    printf("GPIO Interrupt Initialized\n");

    ESP_ERROR_CHECK(lora_set_dio0_mapping(handle, false));
    ESP_ERROR_CHECK(lora_set_mode_rx_continuous(handle));
    printf("Entering RX Continuous Mode...\n");

    uint8_t rx_buf[256];

    while (1) {
        uint8_t ch;
        if (uart_read_bytes(UART_PORT, &ch, 1, 0) > 0) {
            if (ch >= 32 && ch <= 126) {
                printf("Manual TX Trigger received (char: %c)\n", ch);
                const char *msg = "hello";
                size_t msg_len = strlen(msg);
                lora_packet_t pkt = {0};
                pkt.version = 1;
                pkt.type = 1;
                pkt.payload_len = (uint8_t)msg_len;
                memcpy(pkt.payload, msg, msg_len);

                ESP_ERROR_CHECK(lora_send_packet(handle, &pkt));
            }
        }

        if (gpio_check_dio0_and_clear()) {
            uint8_t flags = 0;
            if (lora_get_irq_flags(handle, &flags) != ESP_OK) {
                printf("Failed reading IRQ flags\n");
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }

            if (flags & RFLR_IRQFLAGS_RXDONE) {
                gpio_blink_led();

                uint8_t rx_len = 0;
                if (lora_get_rx_payload_length(handle, &rx_len) != ESP_OK) {
                    printf("Failed reading RX length\n");
                    continue;
                }
                uint16_t bytes_to_read = rx_len;
                if (bytes_to_read > sizeof(rx_buf)) {
                    bytes_to_read = sizeof(rx_buf);
                }

                if (lora_read_fifo_payload(handle, rx_buf, (uint8_t)bytes_to_read) != ESP_OK) {
                    printf("Failed reading RX payload\n");
                    continue;
                }

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

                if (lora_clear_irq_flags(handle, RFLR_IRQFLAGS_RXDONE) != ESP_OK) {
                    printf("Failed clearing RXDONE\n");
                }
            }

            if (flags & RFLR_IRQFLAGS_TXDONE) {
                gpio_blink_led();
                printf("Packet sent successfully (TX Done Interrupt)\n");

                if (lora_clear_irq_flags(handle, RFLR_IRQFLAGS_TXDONE) != ESP_OK) {
                    printf("Failed clearing TXDONE\n");
                }

                if (lora_set_dio0_mapping(handle, false) != ESP_OK) {
                    printf("Failed setting DIO mapping\n");
                }
                if (lora_set_mode_rx_continuous(handle) != ESP_OK) {
                    printf("Failed returning RX continuous\n");
                }
                printf("Returned to RX Continuous Mode\n");
            }

            if (flags & RFLR_IRQFLAGS_PAYLOADCRCERROR) {
                printf("CRC Error in received packet\n");
                if (lora_clear_irq_flags(handle, RFLR_IRQFLAGS_PAYLOADCRCERROR) != ESP_OK) {
                    printf("Failed clearing CRC error flag\n");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
