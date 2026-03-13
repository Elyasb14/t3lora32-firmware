#ifndef LORA_H
#define LORA_H

#include <driver/spi_master.h>
#include <stdint.h>

spi_device_handle_t lora_init();
void lora_reset(void);

uint8_t lora_read_reg(spi_device_handle_t handle, uint8_t reg);
void lora_write_reg(spi_device_handle_t handle, uint8_t reg, uint8_t value);

float lora_get_freq(spi_device_handle_t handle);
uint8_t lora_get_tx_power(spi_device_handle_t handle);

void lora_set_frequency(spi_device_handle_t handle, uint32_t freq_hz);
void lora_set_tx_power(spi_device_handle_t handle, uint8_t dbm);

void lora_set_fifo_tx_base_addr(spi_device_handle_t handle, uint8_t addr);
void lora_write_fifo(spi_device_handle_t handle, const uint8_t *buf,
                     uint8_t len);

void lora_set_mode_standby(spi_device_handle_t handle);
void lora_set_mode_tx(spi_device_handle_t handle);

uint8_t lora_get_irq_flags(spi_device_handle_t handle);
void lora_clear_irq_flags(spi_device_handle_t handle, uint8_t flags);

void lora_send_packet(spi_device_handle_t handle, const uint8_t *buf,
                      uint8_t len);

#endif // LORA_H
