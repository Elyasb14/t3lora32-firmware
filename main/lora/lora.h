#ifndef LORA_H
#define LORA_H

#include "esp_err.h"
#include "sx1276_regs_lora.h"
#include <driver/spi_master.h>
#include <stdbool.h>
#include <stdint.h>

// IRQ flag masks (from SX1276 datasheet)
#define IRQ_TX_DONE_MASK RFLR_IRQFLAGS_TXDONE // 0x08
#define IRQ_RX_DONE_MASK RFLR_IRQFLAGS_RXDONE // 0x40

// Bandwidth values for REG_LR_MODEMCONFIG1 (bits 4-7)
typedef enum {
    LORA_BW_7_81_KHZ = RFLR_MODEMCONFIG1_BW_7_81_KHZ,
    LORA_BW_10_41_KHZ = RFLR_MODEMCONFIG1_BW_10_41_KHZ,
    LORA_BW_15_62_KHZ = RFLR_MODEMCONFIG1_BW_15_62_KHZ,
    LORA_BW_20_83_KHZ = RFLR_MODEMCONFIG1_BW_20_83_KHZ,
    LORA_BW_31_25_KHZ = RFLR_MODEMCONFIG1_BW_31_25_KHZ,
    LORA_BW_41_66_KHZ = RFLR_MODEMCONFIG1_BW_41_66_KHZ,
    LORA_BW_62_50_KHZ = RFLR_MODEMCONFIG1_BW_62_50_KHZ,
    LORA_BW_125_KHZ = RFLR_MODEMCONFIG1_BW_125_KHZ,
    LORA_BW_250_KHZ = RFLR_MODEMCONFIG1_BW_250_KHZ,
    LORA_BW_500_KHZ = RFLR_MODEMCONFIG1_BW_500_KHZ,
} lora_bandwidth_t;
// Spreading Factor values for REG_LR_MODEMCONFIG2 (bits 4-7)
typedef enum {
    LORA_SF_6 = RFLR_MODEMCONFIG2_SF_6,
    LORA_SF_7 = RFLR_MODEMCONFIG2_SF_7,
    LORA_SF_8 = RFLR_MODEMCONFIG2_SF_8,
    LORA_SF_9 = RFLR_MODEMCONFIG2_SF_9,
    LORA_SF_10 = RFLR_MODEMCONFIG2_SF_10,
    LORA_SF_11 = RFLR_MODEMCONFIG2_SF_11,
    LORA_SF_12 = RFLR_MODEMCONFIG2_SF_12,
} lora_spreading_factor_t;
// Coding Rate values for REG_LR_MODEMCONFIG1 (bits 1-3)
typedef enum {
    LORA_CR_4_5 = RFLR_MODEMCONFIG1_CODINGRATE_4_5,
    LORA_CR_4_6 = RFLR_MODEMCONFIG1_CODINGRATE_4_6,
    LORA_CR_4_7 = RFLR_MODEMCONFIG1_CODINGRATE_4_7,
    LORA_CR_4_8 = RFLR_MODEMCONFIG1_CODINGRATE_4_8,
} lora_coding_rate_t;

// initialization and reset
spi_device_handle_t lora_init();
void lora_reset(void);

// register access
esp_err_t lora_read_reg(spi_device_handle_t handle, uint8_t reg, uint8_t *out);
esp_err_t lora_write_reg(spi_device_handle_t handle, uint8_t reg, uint8_t value);

// frequency and power configuration
float lora_get_freq(spi_device_handle_t handle);
uint8_t lora_get_tx_power(spi_device_handle_t handle);
void lora_set_frequency(spi_device_handle_t handle, uint32_t freq_hz);
void lora_set_tx_power(spi_device_handle_t handle, uint8_t dbm);

// FIFO operations
void lora_set_fifo_tx_base_addr(spi_device_handle_t handle, uint8_t addr);
void lora_write_fifo(spi_device_handle_t handle, const uint8_t *buf,
                     uint8_t len);

// mode setting (public helper)
void lora_set_opmode(spi_device_handle_t handle, uint8_t mode);

// operation modes
void lora_set_mode_standby(spi_device_handle_t handle);
void lora_set_mode_tx(spi_device_handle_t handle);

// IRQ handling
uint8_t lora_get_irq_flags(spi_device_handle_t handle);
void lora_clear_irq_flags(spi_device_handle_t handle, uint8_t flags);
void lora_set_dio0_mapping(spi_device_handle_t handle, bool tx_mode);

// transmission
void lora_send_packet(spi_device_handle_t handle, const uint8_t *buf,
                      uint8_t len);

// reception mode helpers
void lora_set_mode_rx_single(spi_device_handle_t handle);
void lora_set_mode_rx_continuous(spi_device_handle_t handle);
bool lora_is_packet_received(spi_device_handle_t handle);
bool lora_is_crc_error(spi_device_handle_t handle);
uint8_t lora_get_rx_payload_length(spi_device_handle_t handle);
void lora_read_fifo_payload(spi_device_handle_t handle, uint8_t *buf, uint8_t len);
void lora_clear_rx_flags(spi_device_handle_t handle);

// reception
int16_t lora_receive_packet(spi_device_handle_t handle, uint8_t *buf,
                            uint16_t buf_size, uint32_t timeout_ms);

void lora_set_bandwidth(spi_device_handle_t handle, lora_bandwidth_t bw);
lora_bandwidth_t lora_get_bandwidth(spi_device_handle_t handle);
void lora_set_spreading_factor(spi_device_handle_t handle, lora_spreading_factor_t sf);
lora_spreading_factor_t lora_get_spreading_factor(spi_device_handle_t handle);
void lora_set_coding_rate(spi_device_handle_t handle, lora_coding_rate_t cr);
lora_coding_rate_t lora_get_coding_rate(spi_device_handle_t handle);

#endif // LORA_H
