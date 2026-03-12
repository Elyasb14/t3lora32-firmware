#include "i2c/i2c.h"
#include "lora/lora.h"
#include "oled/oled.h"
#include <driver/gpio.h>
#include <driver/i2c_types.h>
#include <driver/spi_master.h>
#include <stdint.h>
#include <stdio.h>

#include "i2c/i2c.c"
#include "lora/lora.c"
#include "oled/oled.c"

void app_main() {
  i2c_master_dev_handle_t dev_handle = i2c_init();
  oled_init(dev_handle);

  spi_device_handle_t lora_handle = lora_init();

  uint32_t frf = lora_get_freq(lora_handle);

  char freq_str[16]; // buffer for string
  snprintf(freq_str, sizeof(freq_str), "%.3f MHz", (double)frf);

  oled_draw_string(dev_handle, freq_str, 0, 7);
}
