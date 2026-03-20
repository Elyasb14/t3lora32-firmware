#ifndef OLED_H
#define OLED_H

#include "driver/i2c_master.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define OLED_PAGES 8
#define OLED_COLS 128
#define OLED_CHAR_WIDTH 6
#define OLED_PAGE_CHARS (OLED_COLS / OLED_CHAR_WIDTH)

typedef struct {
    i2c_master_dev_handle_t i2c_handle;
    char pages[OLED_PAGES][OLED_PAGE_CHARS + 1];
    bool dirty;
    int8_t shift_offset;
    uint32_t shift_timer;
} oled_state_t;

oled_state_t oled_init(i2c_master_dev_handle_t handle);
void oled_set_text(oled_state_t *state, uint8_t page, const char *text);
void oled_clear_pages(oled_state_t *state);
void oled_redraw(oled_state_t *state);

void oled_send_cmd(oled_state_t *state, uint8_t cmd);
void oled_send_data(oled_state_t *state, uint8_t *data, size_t len);
void oled_clear_display(oled_state_t *state);
void oled_fill_white(oled_state_t *state);
void oled_draw_char(oled_state_t *state, char c, uint8_t col, uint8_t page);
void oled_draw_string(oled_state_t *state, const char *str, uint8_t col,
                       uint8_t page);

#endif // OLED_H
