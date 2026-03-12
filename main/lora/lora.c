#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "sx1276_regs_lora.h"
#include <freertos/task.h>

#define MOSI 27
#define SCLK 5
#define CS 18
#define DIO 26
#define RST 23
#define MISO 19

// registers from
// https://cdn-shop.adafruit.com/product-files/5714/SX1276-7-8.pdf
#define REG_FIFO 0x00
#define REG_OP_MODE 0x01
#define REG_FIFO_ADDR_PTR 0x0D
#define REG_FIFO_TX_BASE 0x0E
#define REG_FIFO_RX_BASE 0x0F
#define REG_IRQ_FLAGS 0x12
#define REG_RX_NB_BYTES 0x13
#define REG_PKT_SNR 0x19
#define REG_PKT_RSSI 0x1A
#define REG_PAYLOAD_LENGTH 0x22
#define REG_VERSION 0x42

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

spi_device_handle_t lora_init() {
  gpio_config_t io_conf = {
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = (1ULL << RST),
  };

  gpio_config(&io_conf);

  spi_device_handle_t handle = _lora_spi_init();
  lora_reset();

  // sleep
  lora_write_reg(handle, REG_OP_MODE, 0x00);

  // LoRa mode
  lora_write_reg(handle, REG_OP_MODE, 0x80);

  // frequency (915MHz example)
  lora_write_reg(handle, REG_LR_FRFMSB, 0xE4);
  lora_write_reg(handle, REG_LR_FRFMID, 0xC0);
  lora_write_reg(handle, REG_LR_FRFLSB, 0x00);

  // standby
  lora_write_reg(handle, REG_OP_MODE, 0x81);

  return handle;
}
