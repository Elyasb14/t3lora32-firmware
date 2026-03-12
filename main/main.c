#include "driver/i2c_types.h"
#include "driver/spi_master.h"
#include "i2c/i2c_helpers.h"
#include "lora/lora.h"
#include "oled/oled.h"
#include <driver/gpio.h>

#include "i2c/i2c_helpers.c"
#include "lora/lora.c"
#include "oled/oled.c"

void app_main() {
  i2c_master_dev_handle_t dev_handle = i2c_init();
  oled_init(dev_handle);
  oled_draw_string(dev_handle, "hello", 0, 7);

  spi_device_handle_t handle = lora_init();
}
