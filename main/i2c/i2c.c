#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_SCL_IO 22
#define OLED_ADDR 0x3C

/// init I2C master bus, add OLED device, and return device handle
i2c_master_dev_handle_t i2c_init() {
  // TODO: break this into two functions to get bus_handle and for dev_handle?
  i2c_master_bus_config_t i2c_mst_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = -1,
      .scl_io_num = I2C_MASTER_SCL_IO,
      .sda_io_num = I2C_MASTER_SDA_IO,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };

  i2c_master_bus_handle_t bus_handle;
  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = OLED_ADDR,
      .scl_speed_hz = 100000,
  };

  i2c_master_dev_handle_t dev_handle;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
  ESP_ERROR_CHECK(i2c_master_probe(bus_handle, OLED_ADDR, 10));
  return dev_handle;
}
