#include "gpio/gpio.h"
#include "i2c/i2c.h"
#include "lora/lora.h"
#include "net/identity.h"
#include "net/net.h"
#include "net/packet.h"
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
#include "net/identity.c"
#include "oled/oled.c"

#define TX_TRIGGER_CHAR 't'
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

void send_packet_manual(spi_device_handle_t handle, oled_state_t *oled, node_identity_t *identity);

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

    node_identity_t identity;
    if (identity_init(&identity) != ESP_OK) {
        printf("Failed to initialize identity!\n");
        return;
    }

    printf("Node ID: ");
    for (int i = 0; i < NODE_ID_LEN; i++) {
        printf("%02X", identity.node_id[i]);
    }
    printf("\n");

    i2c_master_dev_handle_t i2c_handle = i2c_init();
    oled_state_t oled = oled_init(i2c_handle);

    spi_device_handle_t handle = lora_init();

    lora_set_tx_power(handle, 1);
    lora_set_frequency(handle, 903000000);
    lora_set_bandwidth(handle, LORA_BW_125_KHZ);

    char id_buffer[24];
    char bw_buffer[32];
    char tx_buffer[32];
    char freq_buffer[32];

    snprintf(id_buffer, sizeof(id_buffer), "ID: %02X%02X%02X%02X",
             identity.node_id[0], identity.node_id[1],
             identity.node_id[2], identity.node_id[3]);
    snprintf(bw_buffer, sizeof(bw_buffer), "BW: %d kHz", lora_get_bandwidth(handle));
    snprintf(tx_buffer, sizeof(tx_buffer), "TX: %d dBm", lora_get_tx_power(handle));
    snprintf(freq_buffer, sizeof(freq_buffer), "Freq: %.2f MHz", lora_get_freq(handle));

    oled_clear_pages(&oled);
    oled_set_text(&oled, 2, id_buffer);
    oled_set_text(&oled, 3, "mesh repeater");
    oled_set_text(&oled, 4, freq_buffer);
    oled_set_text(&oled, 5, bw_buffer);
    oled_set_text(&oled, 6, tx_buffer);

    xTaskCreatePinnedToCore(oled_task, "oled_task", 3072, &oled,
                            configMAX_PRIORITIES - 3, NULL, 1);

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
                send_packet_manual(handle, &oled, &identity);
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

static uint32_t g_seq_num = 0;

void send_mesh_packet(spi_device_handle_t handle, node_identity_t *identity,
                      const uint8_t *to_id, uint8_t type,
                      const uint8_t *payload, size_t payload_len) {
    size_t header_len = 1 + NODE_ID_LEN + NODE_ID_LEN + 4 + PUBKEY_LEN;
    size_t packet_len = header_len + SIG_LEN + payload_len;
    uint8_t packet[packet_len];

    mesh_packet_t *pkt = (mesh_packet_t *)packet;
    pkt->type = type;
    memcpy(pkt->from_id, identity->node_id, NODE_ID_LEN);
    memcpy(pkt->to_id, to_id, NODE_ID_LEN);
    pkt->seq_num = g_seq_num++;
    memcpy(pkt->public_key, identity->public_key, PUBKEY_LEN);

    size_t sig_input_len = header_len + payload_len;
    uint8_t sig_input[sig_input_len];
    memcpy(sig_input, pkt, header_len);
    memcpy(sig_input + header_len, payload, payload_len);

    if (identity_sign(identity, sig_input, sig_input_len, pkt->signature) != ESP_OK) {
        printf("Failed to sign packet\n");
        return;
    }

    memcpy(packet + header_len + SIG_LEN, payload, payload_len);

    lora_set_dio0_mapping(handle, true);
    lora_set_mode_standby(handle);
    lora_write_reg(handle, REG_LR_FIFOTXBASEADDR, 0x00);
    lora_write_reg(handle, REG_LR_FIFOADDRPTR, 0x00);
    lora_write_reg(handle, REG_LR_PAYLOADLENGTH, packet_len);

    for (size_t i = 0; i < packet_len; i++) {
        lora_write_reg(handle, REG_LR_FIFO, packet[i]);
    }

    lora_set_mode_tx(handle);
}

void send_packet_manual(spi_device_handle_t handle, oled_state_t *oled, node_identity_t *identity) {
    uint8_t broadcast_id[NODE_ID_LEN] = {0};
    char *data = "hello";
    printf("Transmitting: '%s'\n", data);

    send_mesh_packet(handle, identity, broadcast_id, PKT_TYPE_TEXT,
                     (uint8_t *)data, strlen(data));
    printf("TX Mode Started, waiting for interrupt...\n");
}
