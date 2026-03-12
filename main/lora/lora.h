#ifndef LORA_H
#define LORA_H

#include <driver/spi_master.h>
#include <stdint.h>

void lora_reset(void);
uint8_t lora_read_reg(spi_device_handle_t handle, uint8_t reg);
void lora_write_reg(spi_device_handle_t handle, uint8_t reg, uint8_t value);
spi_device_handle_t lora_init(void);
float lora_get_freq(spi_device_handle_t handle);

#endif // LORA_H
