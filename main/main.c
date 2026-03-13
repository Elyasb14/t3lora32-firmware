#include "i2c/i2c.h"
#include "lora/lora.h"
#include "oled/oled.h"
#include <driver/gpio.h>
#include <driver/i2c_types.h>
#include <driver/spi_master.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "i2c/i2c.c"
#include "lora/lora.c"
#include "oled/oled.c"

void app_main() {
    char *data = "12345";
    spi_device_handle_t handle = lora_init();

    lora_set_tx_power(handle, 17);
    uint8_t tx_power = lora_get_tx_power(handle);

    lora_set_frequency(handle, 903000000);
    float freq = lora_get_freq(handle);

    printf("TX_POWER: %d\n", tx_power);
    printf("FREQ: %f\n", freq);

    lora_send_packet(handle, (uint8_t *)data, strlen(data));

    printf("Packet sent successfully\n");
}
