#include "lora/lora.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "sx1276_regs_lora.h"
#include <assert.h>
#include <freertos/task.h>
#include <math.h>
#include <stdint.h>

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
spi_device_handle_t lora_init(uint32_t freq_hz) {
  gpio_config_t io_conf = {
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = (1ULL << RST),
  };

  gpio_config(&io_conf);

  spi_device_handle_t handle = _lora_spi_init();
  lora_reset();

  // LoRa mode
  lora_write_reg(handle, REG_LR_OPMODE, 0x80);

  lora_set_frequency(handle, freq_hz);

  // standby
  lora_write_reg(handle, REG_LR_OPMODE, 0x81);

  uint8_t v = lora_read_reg(handle, REG_LR_VERSION);
  assert(v == 0x12 || v == 0x11);
  return handle;
}

float lora_get_freq(spi_device_handle_t handle) {
  uint32_t frf = (lora_read_reg(handle, 0x06) << 16) |
                 (lora_read_reg(handle, 0x07) << 8) |
                 (lora_read_reg(handle, 0x08));

  return (float)frf * 61.03515625 / 1e6;
}

// https://cdn-shop.adafruit.com/product-files/5714/SX1276-7-8.pdf page 32
void lora_set_frequency(spi_device_handle_t handle, uint32_t freq_hz) {
  uint32_t frf = ((uint64_t)freq_hz << 19) / 32000000;

  lora_write_reg(handle, REG_LR_FRFMSB, (frf >> 16) & 0xFF);
  lora_write_reg(handle, REG_LR_FRFMID, (frf >> 8) & 0xFF);
  lora_write_reg(handle, REG_LR_FRFLSB, frf & 0xFF);
}

void lora_set_tx_power(spi_device_handle_t handle, uint8_t dbm) {
  uint8_t pa_config = 0;

  pa_config |= (1 << 7);  // PA_BOOST, max power of +20 dBm
  pa_config |= (7 << 4);  // MaxPower
  pa_config |= (dbm - 2); // OutputPower
  lora_write_reg(handle, REG_LR_PACONFIG, pa_config);
}

uint8_t lora_get_tx_power(spi_device_handle_t handle) {
  uint8_t reg = lora_read_reg(handle, REG_LR_PACONFIG);
  return (reg & 0x0F) + 2; // power is lower 4 bits of PaConfig register,
                           // convert it to dbm by adding 2
}
