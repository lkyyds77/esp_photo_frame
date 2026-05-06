#pragma once

#include "esp_err.h"
#include <stdint.h>

esp_err_t lcd_display_init(void);
void lcd_draw_bitmap(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t *color_data);
