#include "i2c/i2c.h"
#include "lora/lora.h"
#include "oled/oled.h"
#include <driver/gpio.h>
#include <driver/i2c_types.h>
#include <driver/spi_master.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "i2c/i2c.c"
#include "lora/lora.c"
#include "oled/oled.c"

// Configuration: Uncomment the mode you want to test
#define LORA_TEST_MODE_TX
// #define LORA_TEST_MODE_RX

void app_main() {
    spi_device_handle_t handle = lora_init();

    // Configure LoRa parameters
    lora_set_tx_power(handle, 17);
    lora_set_frequency(handle, 903000000);

    printf("TX Power: %d dBm\n", lora_get_tx_power(handle));
    printf("Frequency: %.2f MHz\n", lora_get_freq(handle));

#ifdef LORA_TEST_MODE_TX
    // --- TRANSMITTER CODE ---
    char *data = "12345";
    printf("Transmitting: '%s'\n", data);

    lora_send_packet(handle, (uint8_t *)data, 5);

    printf("Packet sent successfully\n");

#elif defined(LORA_TEST_MODE_RX)
    // --- RECEIVER CODE ---
    uint8_t rx_buf[256]; // Caller provides buffer (max 256 bytes)
    printf("Entering receive mode...\n");

    while (1) {
        // Wait for packet with 5-second timeout
        int16_t rx_len = lora_receive_packet(handle, rx_buf, sizeof(rx_buf), 5000);

        if (rx_len > 0) {
            // 1. Print Hex Dump
            printf("Received %d bytes (Hex): ", rx_len);
            for (int i = 0; i < rx_len; i++) {
                printf("%02X ", rx_buf[i]);
            }
            printf("\n");

            // 2. Print ASCII String (printable chars only)
            printf("Received String: '");
            for (int i = 0; i < rx_len; i++) {
                if (rx_buf[i] >= 32 && rx_buf[i] <= 126) {
                    printf("%c", rx_buf[i]);
                } else {
                    printf("."); // Replace non-printable with dot
                }
            }
            printf("'\n");
        } else if (rx_len == -1) {
            printf("Timeout waiting for packet\n");
        } else if (rx_len == -2) {
            printf("CRC Error in received packet\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Prevent CPU hogging
    }
#endif
}
