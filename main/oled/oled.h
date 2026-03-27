#ifndef OLED_H
#define OLED_H

#include <driver/i2c_types.h>
#include <stddef.h>
#include <stdint.h>

#define OLED_PAGES 8
#define OLED_COLS 128
#define OLED_CHAR_WIDTH 6
#define OLED_PAGE_CHARS (OLED_COLS / OLED_CHAR_WIDTH)

void oled_init(i2c_master_dev_handle_t handle);
void oled_clear_display(i2c_master_dev_handle_t handle);
void oled_fill_white(i2c_master_dev_handle_t handle);
void oled_draw_char(i2c_master_dev_handle_t handle, char c, uint8_t col,
                    uint8_t page);
void oled_draw_string(i2c_master_dev_handle_t handle, const char *str,
                      uint8_t col, uint8_t page);

#endif // OLED_H
