#ifndef LORA_H
#define LORA_H

#include <driver/spi_master.h>
#include <stdint.h>

spi_device_handle_t lora_init(uint32_t freq_hz);
void lora_reset(void);
uint8_t lora_read_reg(spi_device_handle_t handle, uint8_t reg);
void lora_write_reg(spi_device_handle_t handle, uint8_t reg, uint8_t value);
float lora_get_freq(spi_device_handle_t handle);
uint8_t lora_get_tx_power(spi_device_handle_t handle);

void lora_set_frequency(spi_device_handle_t handle, uint32_t freq_hz);
void lora_set_tx_power(spi_device_handle_t handle, uint8_t dbm);
void lora_set_spreading_factor(spi_device_handle_t handle, uint8_t sf);
void lora_set_bandwidth(spi_device_handle_t handle, uint8_t bw);
void lora_set_coding_rate(spi_device_handle_t handle, uint8_t cr);
void lora_set_preamble_length(spi_device_handle_t handle, uint16_t len);

#endif // LORA_H
