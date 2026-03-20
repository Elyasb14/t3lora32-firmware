#include "lora/lora.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "sx1276_regs_lora.h"
#include <assert.h>
#include <freertos/task.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MOSI 27
#define SCLK 5
#define CS 18
#define DIO 26
#define RST 23
#define MISO 19

void lora_reset() {
    gpio_set_level(RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

spi_device_handle_t _lora_spi_init() {
    spi_device_handle_t lora_spi;
    spi_bus_config_t buscfg = {
        .mosi_io_num = MOSI,
        .miso_io_num = MISO,
        .sclk_io_num = SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,
        .mode = 0,
        .spics_io_num = CS,
        .queue_size = 1,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &lora_spi));
    return lora_spi;
}

uint8_t lora_read_reg(spi_device_handle_t handle, uint8_t reg) {
    spi_transaction_t t = {0};

    uint8_t tx[2] = {reg & 0x7F, 0};
    uint8_t rx[2] = {0};

    t.length = 16;
    t.tx_buffer = tx;
    t.rx_buffer = rx;

    spi_device_transmit(handle, &t);

    return rx[1];
}

void lora_write_reg(spi_device_handle_t handle, uint8_t reg, uint8_t value) {
    spi_transaction_t t = {0};

    uint8_t tx[2] = {reg | 0x80, value};

    t.length = 16;
    t.tx_buffer = tx;

    spi_device_transmit(handle, &t);
}

void lora_set_frequency(spi_device_handle_t handle, uint32_t freq_hz);
spi_device_handle_t lora_init() {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << RST),
    };

    gpio_config(&io_conf);

    spi_device_handle_t handle = _lora_spi_init();
    lora_reset();

    // LoRa mode
    lora_write_reg(handle, REG_LR_OPMODE, 0x80);

    // default freq is 915 mhz
    lora_write_reg(handle, REG_LR_FRFMSB, 0xE4);
    lora_write_reg(handle, REG_LR_FRFMID, 0xC0);
    lora_write_reg(handle, REG_LR_FRFLSB, 0x00);
    // standby
    lora_write_reg(handle, REG_LR_OPMODE, 0x81);

    uint8_t v = lora_read_reg(handle, REG_LR_VERSION);
    assert(v == 0x12 || v == 0x11);
    return handle;
}

float lora_get_freq(spi_device_handle_t handle) {
    uint32_t frf = (lora_read_reg(handle, REG_LR_FRFMSB) << 16) |
                   (lora_read_reg(handle, REG_LR_FRFMID) << 8) |
                   (lora_read_reg(handle, REG_LR_FRFLSB));

    return (float)frf * 61.03515625 / 1e6;
}

// https://cdn-shop.adafruit.com/product-files/5714/SX1276-7-8.pdf page 32
// Formula: Frf = (Freq * 2^19) / Fxosc
// Fxosc = 32 MHz (SX1276 crystal frequency)
void lora_set_frequency(spi_device_handle_t handle, uint32_t freq_hz) {
    uint32_t frf = ((uint64_t)freq_hz << 19) / 32000000;

    lora_write_reg(handle, REG_LR_FRFMSB, (frf >> 16) & 0xFF);
    lora_write_reg(handle, REG_LR_FRFMID, (frf >> 8) & 0xFF);
    lora_write_reg(handle, REG_LR_FRFLSB, frf & 0xFF);
}

void lora_set_tx_power(spi_device_handle_t handle, uint8_t dbm) {
    // SX1276 datasheet: PA_BOOST output power range is 2-17 dBm
    // Pout = Pmax - 15 + OutputPower (where Pmax = 15 dBm for PA_BOOST)
    if (dbm < 2) dbm = 2;
    if (dbm > 17) dbm = 17;

    uint8_t pa_config = 0;

    // Select PA_BOOST pin (bit 7)
    pa_config |= RFLR_PACONFIG_PASELECT_PABOOST;

    // Set MaxPower (bits 6-4): 111 = 15 dBm (Pmax)
    pa_config |= (7 << 4);

    // Set OutputPower (bits 3-0): Pout = dbm - 2
    pa_config |= (dbm - 2);

    lora_write_reg(handle, REG_LR_PACONFIG, pa_config);
}

uint8_t lora_get_tx_power(spi_device_handle_t handle) {
    uint8_t reg = lora_read_reg(handle, REG_LR_PACONFIG);
    return (reg & 0x0F) + 2; // power is lower 4 bits of PaConfig register,
                             // convert it to dbm by adding 2
}

// Helper function to set operation mode using datasheet constants
void lora_set_opmode(spi_device_handle_t handle, uint8_t mode) {
    uint8_t opmode = lora_read_reg(handle, REG_LR_OPMODE);
    opmode = (opmode & RFLR_OPMODE_MASK) | mode;
    lora_write_reg(handle, REG_LR_OPMODE, opmode);
}

void lora_set_mode_standby(spi_device_handle_t handle) {
    lora_set_opmode(handle, RFLR_OPMODE_STANDBY);
}

void lora_set_mode_tx(spi_device_handle_t handle) {
    lora_set_opmode(handle, RFLR_OPMODE_TRANSMITTER);
}

void lora_set_fifo_tx_base_addr(spi_device_handle_t handle, uint8_t addr) {
    lora_write_reg(handle, REG_LR_FIFOTXBASEADDR, addr);
    lora_write_reg(handle, REG_LR_FIFOADDRPTR, addr);
}

void lora_write_fifo(spi_device_handle_t handle, const uint8_t *buf,
                     uint8_t len) {
    lora_write_reg(handle, REG_LR_PAYLOADLENGTH, len);

    for (int i = 0; i < len; i++) {
        lora_write_reg(handle, REG_LR_FIFO, buf[i]);
    }
}

uint8_t lora_get_irq_flags(spi_device_handle_t handle) {
    return lora_read_reg(handle, REG_LR_IRQFLAGS);
}

void lora_clear_irq_flags(spi_device_handle_t handle, uint8_t flags) {
    // Writing 1 to IRQ flags clears them (per datasheet)
    lora_write_reg(handle, REG_LR_IRQFLAGS, flags);
}

void lora_set_dio0_mapping(spi_device_handle_t handle, bool tx_mode) {
    uint8_t current_mapping = lora_read_reg(handle, REG_LR_DIOMAPPING1);

    // Clear DIO0 bits (bits 6-7)
    current_mapping &= RFLR_DIOMAPPING1_DIO0_MASK;

    if (tx_mode) {
        // set DIO0 to TxDone (01)
        current_mapping |= RFLR_DIOMAPPING1_DIO0_01;
    } else {
        // set DIO0 to RxDone (00) - this is the default
        current_mapping |= RFLR_DIOMAPPING1_DIO0_00;
    }

    lora_write_reg(handle, REG_LR_DIOMAPPING1, current_mapping);
}

void lora_send_packet(spi_device_handle_t handle, const uint8_t *buf,
                      uint8_t len) {
    lora_set_mode_standby(handle);

    lora_write_reg(handle, REG_LR_FIFOTXBASEADDR, 0x00);
    lora_write_reg(handle, REG_LR_FIFOADDRPTR, 0x00);

    lora_write_reg(handle, REG_LR_PAYLOADLENGTH, len);

    for (int i = 0; i < len; i++) {
        lora_write_reg(handle, REG_LR_FIFO, buf[i]);
    }

    lora_set_mode_tx(handle);

    // Wait for TX_DONE interrupt (max timeout ~2 seconds)
    uint32_t timeout = 2000; // ms
    uint32_t start = xTaskGetTickCount() * portTICK_PERIOD_MS;

    while ((lora_get_irq_flags(handle) & RFLR_IRQFLAGS_TXDONE) == 0) {
        if ((xTaskGetTickCount() * portTICK_PERIOD_MS - start) > timeout) {
            // Timeout - abort
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    lora_clear_irq_flags(handle, RFLR_IRQFLAGS_TXDONE);

    lora_set_mode_standby(handle);
}

void lora_set_mode_rx_single(spi_device_handle_t handle) {
    lora_set_mode_standby(handle);
    lora_write_reg(handle, REG_LR_FIFORXBASEADDR, 0x00);
    lora_write_reg(handle, REG_LR_FIFOADDRPTR, 0x00);
    lora_set_opmode(handle, RFLR_OPMODE_RECEIVER_SINGLE);
}

void lora_set_mode_rx_continuous(spi_device_handle_t handle) {
    lora_set_mode_standby(handle);
    lora_write_reg(handle, REG_LR_FIFORXBASEADDR, 0x00);
    lora_write_reg(handle, REG_LR_FIFOADDRPTR, 0x00);
    lora_set_opmode(handle, RFLR_OPMODE_RECEIVER);
}

bool lora_is_packet_received(spi_device_handle_t handle) {
    return (lora_get_irq_flags(handle) & RFLR_IRQFLAGS_RXDONE) != 0;
}

bool lora_is_crc_error(spi_device_handle_t handle) {
    return (lora_get_irq_flags(handle) & RFLR_IRQFLAGS_PAYLOADCRCERROR) != 0;
}

uint8_t lora_get_rx_payload_length(spi_device_handle_t handle) {
    return lora_read_reg(handle, REG_LR_RXNBBYTES);
}

void lora_read_fifo_payload(spi_device_handle_t handle, uint8_t *buf, uint8_t len) {
    for (int i = 0; i < len; i++) {
        buf[i] = lora_read_reg(handle, REG_LR_FIFO);
    }
}

void lora_clear_rx_flags(spi_device_handle_t handle) {
    lora_clear_irq_flags(handle, RFLR_IRQFLAGS_RXDONE | RFLR_IRQFLAGS_PAYLOADCRCERROR);
}

// returns -2: crc error
// returns -1: timeout
int16_t lora_receive_packet(spi_device_handle_t handle, uint8_t *buf,
                            uint16_t buf_size, uint32_t timeout_ms) {
    // fifo buffer is 256 bytes
    if (buf_size > 256) {
        buf_size = 256;
    }

    lora_set_mode_rx_single(handle);

    uint32_t start = xTaskGetTickCount() * portTICK_PERIOD_MS;

    while (!lora_is_packet_received(handle)) {
        if (timeout_ms > 0) {
            uint32_t elapsed = (xTaskGetTickCount() * portTICK_PERIOD_MS) - start;
            if (elapsed > timeout_ms) {
                lora_clear_irq_flags(handle, RFLR_IRQFLAGS_RXTIMEOUT);
                return -1;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    if (lora_is_crc_error(handle)) {
        lora_clear_rx_flags(handle);
        return -2;
    }

    uint8_t rx_len = lora_get_rx_payload_length(handle);
    uint16_t bytes_to_read = (rx_len > buf_size) ? buf_size : rx_len;

    if (bytes_to_read > 256) {
        bytes_to_read = 256;
    }

    lora_read_fifo_payload(handle, buf, (uint8_t)bytes_to_read);
    lora_clear_rx_flags(handle);
    lora_set_mode_standby(handle);

    return (int16_t)bytes_to_read;
}

void lora_set_bandwidth(spi_device_handle_t handle, lora_bandwidth_t bw) {
    uint8_t reg = lora_read_reg(handle, REG_LR_MODEMCONFIG1);
    reg = (reg & RFLR_MODEMCONFIG1_BW_MASK) | bw;
    lora_write_reg(handle, REG_LR_MODEMCONFIG1, reg);
}

lora_bandwidth_t lora_get_bandwidth(spi_device_handle_t handle) {
    uint8_t val = lora_read_reg(handle, REG_LR_MODEMCONFIG1);
    val &= 0xf0;
    return (lora_bandwidth_t)val;
}

void lora_set_coding_rate(spi_device_handle_t handle, lora_coding_rate_t cr) {
    uint8_t reg = lora_read_reg(handle, REG_LR_MODEMCONFIG1);
    reg = (reg & RFLR_MODEMCONFIG1_CODINGRATE_MASK) | cr;
    lora_write_reg(handle, REG_LR_MODEMCONFIG1, reg);
}

void lora_set_spreading_factor(spi_device_handle_t handle, lora_spreading_factor_t sf) {
    uint8_t reg = lora_read_reg(handle, REG_LR_MODEMCONFIG2);
    reg = (reg & RFLR_MODEMCONFIG2_SF_MASK) | sf;
    lora_write_reg(handle, REG_LR_MODEMCONFIG2, reg);
}

lora_spreading_factor_t lora_get_spreading_factor(spi_device_handle_t handle) {
    uint8_t val = lora_read_reg(handle, REG_LR_MODEMCONFIG2);
    val &= 0xF0;
    return (lora_spreading_factor_t)val;
}

lora_coding_rate_t lora_get_coding_rate(spi_device_handle_t handle) {
    uint8_t val = lora_read_reg(handle, REG_LR_MODEMCONFIG1);
    val &= 0x0E;
    return (lora_coding_rate_t)val;
}
