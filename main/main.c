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

    spi_device_handle_t lora_handle = lora_init(903000000);

    float frf = lora_get_freq(lora_handle);
    char freq_str[16];
    snprintf(freq_str, sizeof(freq_str), "%.3f MHz", frf);

    lora_set_tx_power(lora_handle, 17);

    uint8_t tx_power = lora_get_tx_power(lora_handle);

    char tx_power_str[16];
    snprintf(tx_power_str, sizeof(tx_power_str), "%.3f dBm", (double)tx_power);

    oled_draw_string(dev_handle, freq_str, 0, 7);
    oled_draw_string(dev_handle, tx_power_str, 0, 6);
}
