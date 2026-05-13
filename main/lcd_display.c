#include "lcd_display.h"
#include "hardware_config.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "LCD_DISPLAY";

static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static SemaphoreHandle_t s_flush_done_sem = NULL;

static bool lcd_color_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    (void)panel_io;
    (void)edata;
    SemaphoreHandle_t sem = (SemaphoreHandle_t)user_ctx;
    BaseType_t high_task_wakeup = pdFALSE;
    if (sem) {
        xSemaphoreGiveFromISR(sem, &high_task_wakeup);
    }
    return high_task_wakeup == pdTRUE;
}

esp_err_t lcd_display_init(void)
{
    esp_err_t ret = shared_spi_bus_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus.");
        return ret;
    }

    if (!s_flush_done_sem) {
        s_flush_done_sem = xSemaphoreCreateBinary();
        if (!s_flush_done_sem) {
            ESP_LOGE(TAG, "Failed to create flush done semaphore");
            return ESP_ERR_NO_MEM;
        }
    }

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = 20 * 1000 * 1000,   // 继续降速到 20MHz，确保使用飞线时信号稳定不出杂点
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = lcd_color_trans_done_cb,
        .user_ctx = s_flush_done_sem,
    };

    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SHARED_SPI_HOST, &io_config, &s_io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        // Make panel accept little-endian RGB565 so we can DMA raw buffers without CPU byte swapping
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 16,
    };

    ESP_LOGI(TAG, "Install ST7789 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(s_io_handle, &panel_config, &s_panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));
    
    // ST7789 screens usually need color inversion
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel_handle, true));
    
    // Turn on the screen
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));

    return ESP_OK;
}

void lcd_draw_bitmap(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t *color_data) {
    (void)lcd_draw_bitmap_dma(x0, y0, w, h, color_data, portMAX_DELAY);
}

esp_err_t lcd_draw_bitmap_dma(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, const void *color_data, TickType_t timeout)
{
    if (!s_panel_handle || !s_flush_done_sem) {
        return ESP_ERR_INVALID_STATE;
    }

    // Drain any stale completion signals
    while (xSemaphoreTake(s_flush_done_sem, 0) == pdTRUE) {
    }

    esp_err_t ret = esp_lcd_panel_draw_bitmap(s_panel_handle, x0, y0, x0 + w, y0 + h, color_data);
    if (ret != ESP_OK) {
        return ret;
    }

    if (xSemaphoreTake(s_flush_done_sem, timeout) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}
