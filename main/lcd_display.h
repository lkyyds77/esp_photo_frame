#pragma once

#include "esp_err.h"
#include <stdint.h>

#include "freertos/FreeRTOS.h"

esp_err_t lcd_display_init(void);
void lcd_draw_bitmap(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t *color_data);

// Queue one DMA transfer and wait for completion (signaled by panel IO callback)
esp_err_t lcd_draw_bitmap_dma(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, const void *color_data, TickType_t timeout);
