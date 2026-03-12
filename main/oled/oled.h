#ifndef OLED_H
#define OLED_H

#include "driver/i2c_master.h"
#include <stddef.h>
#include <stdint.h>

void oled_send_cmd(i2c_master_dev_handle_t handle, uint8_t cmd);
void oled_send_data(i2c_master_dev_handle_t handle, uint8_t *data, size_t len);
void oled_clear_display(i2c_master_dev_handle_t handle);
void oled_init(i2c_master_dev_handle_t handle);
void oled_fill_white(i2c_master_dev_handle_t handle);
void oled_draw_char(i2c_master_dev_handle_t handle, char c, uint8_t col,
                    uint8_t page);
void oled_draw_string(i2c_master_dev_handle_t handle, const char *str,
                      uint8_t col, uint8_t page);

#endif // OLED_H
