#ifndef LORA_H
#define LORA_H

#include <driver/spi_master.h>
#include <stdint.h>
#include <stdbool.h>

// IRQ flag masks (from SX1276 datasheet)
#define IRQ_TX_DONE_MASK RFLR_IRQFLAGS_TXDONE // 0x08
#define IRQ_RX_DONE_MASK RFLR_IRQFLAGS_RXDONE // 0x40

// Initialization and reset
spi_device_handle_t lora_init();
void lora_reset(void);

// Register access
uint8_t lora_read_reg(spi_device_handle_t handle, uint8_t reg);
void lora_write_reg(spi_device_handle_t handle, uint8_t reg, uint8_t value);

// Frequency and power configuration
float lora_get_freq(spi_device_handle_t handle);
uint8_t lora_get_tx_power(spi_device_handle_t handle);
void lora_set_frequency(spi_device_handle_t handle, uint32_t freq_hz);
void lora_set_tx_power(spi_device_handle_t handle, uint8_t dbm);

// FIFO operations
void lora_set_fifo_tx_base_addr(spi_device_handle_t handle, uint8_t addr);
void lora_write_fifo(spi_device_handle_t handle, const uint8_t *buf,
                     uint8_t len);

// Mode setting (public helper)
void lora_set_opmode(spi_device_handle_t handle, uint8_t mode);

// Operation modes
void lora_set_mode_standby(spi_device_handle_t handle);
void lora_set_mode_tx(spi_device_handle_t handle);

// IRQ handling
uint8_t lora_get_irq_flags(spi_device_handle_t handle);
void lora_clear_irq_flags(spi_device_handle_t handle, uint8_t flags);
void lora_set_dio0_mapping(spi_device_handle_t handle, bool tx_mode);

// Transmission
void lora_send_packet(spi_device_handle_t handle, const uint8_t *buf,
                      uint8_t len);

// Reception mode helpers
void lora_set_mode_rx_single(spi_device_handle_t handle);
void lora_set_mode_rx_continuous(spi_device_handle_t handle);
bool lora_is_packet_received(spi_device_handle_t handle);
bool lora_is_crc_error(spi_device_handle_t handle);
uint8_t lora_get_rx_payload_length(spi_device_handle_t handle);
void lora_read_fifo_payload(spi_device_handle_t handle, uint8_t *buf, uint8_t len);
void lora_clear_rx_flags(spi_device_handle_t handle);

// Reception
int16_t lora_receive_packet(spi_device_handle_t handle, uint8_t *buf,
                            uint16_t buf_size, uint32_t timeout_ms);

#endif // LORA_H
