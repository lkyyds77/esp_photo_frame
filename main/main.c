#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "wifi_ap.h"
#include "image_server.h"
#include "sd_card.h"
#include "lcd_display.h"
#include "hardware_config.h"
#include "app_context.h"

#include "esp_heap_caps.h"

static const char *TAG = "main";

static void photo_slideshow_task(void *arg)
{
    (void)arg;

    const size_t frame_bytes = (size_t)LCD_H_RES * (size_t)LCD_V_RES * 2;
    uint8_t *img_buf = (uint8_t *)heap_caps_malloc(frame_bytes, MALLOC_CAP_DMA);
    if (!img_buf) {
        ESP_LOGE(TAG, "Failed to allocate DMA-capable image buffer (%u bytes)", (unsigned)frame_bytes);
        vTaskDelete(NULL);
        return;
    }

    // Wait until SD and LCD are both ready
    xEventGroupWaitBits(g_app_events, APP_EVT_SD_READY | APP_EVT_LCD_READY, pdFALSE, pdTRUE, portMAX_DELAY);

    while (1) {
        bool photo_found = false;

        for (int i = 0; i < 10; i++) {
            char filename[32];
            snprintf(filename, sizeof(filename), "/sdcard/photo_%d.rgb", i);

            if (xSemaphoreTake(g_shared_bus_mutex, pdMS_TO_TICKS(30000)) != pdTRUE) {
                continue;
            }

            FILE *f = fopen(filename, "rb");
            if (!f) {
                xSemaphoreGive(g_shared_bus_mutex);
                continue;
            }

            photo_found = true;

            size_t bytes_read = fread(img_buf, 1, frame_bytes, f);
            fclose(f);
            xSemaphoreGive(g_shared_bus_mutex);

            if (bytes_read == 0) {
                continue;
            }
            if (bytes_read < frame_bytes) {
                memset(img_buf + bytes_read, 0, frame_bytes - bytes_read);
            }

            // LCD refresh via SPI DMA (no per-pixel CPU endianness swap)
            if (xSemaphoreTake(g_shared_bus_mutex, pdMS_TO_TICKS(30000)) == pdTRUE) {
                esp_err_t err = lcd_draw_bitmap_dma(0, 0, LCD_H_RES, LCD_V_RES, img_buf, pdMS_TO_TICKS(5000));
                xSemaphoreGive(g_shared_bus_mutex);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "LCD flush failed: %s", esp_err_to_name(err));
                }
            }

            // Show each photo up to 5s, but wake early if a new photo arrives
            EventBits_t bits = xEventGroupWaitBits(g_app_events, APP_EVT_NEW_PHOTO, pdTRUE, pdFALSE, pdMS_TO_TICKS(5000));
            if (bits & APP_EVT_NEW_PHOTO) {
                break;
            }
        }

        if (!photo_found) {
            // Avoid tight loop when SD is empty
            xEventGroupWaitBits(g_app_events, APP_EVT_NEW_PHOTO, pdTRUE, pdFALSE, pdMS_TO_TICKS(1000));
        }
    }
}

void app_main(void)
{
    // 1. 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starting ESP Photo Frame Application");

    app_context_init();
    if (!g_app_events || !g_shared_bus_mutex) {
        ESP_LOGE(TAG, "Failed to init app sync objects");
        return;
    }

    // Initialize SD card
    ESP_LOGI(TAG, "Initializing SD Card...");
    esp_err_t err = sd_card_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SD init failed: %s", esp_err_to_name(err));
        return;
    }
    xEventGroupSetBits(g_app_events, APP_EVT_SD_READY);

    // Initialize LCD
    ESP_LOGI(TAG, "Initializing LCD...");
    err = lcd_display_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LCD init failed: %s", esp_err_to_name(err));
        return;
    }
    xEventGroupSetBits(g_app_events, APP_EVT_LCD_READY);

    // 2. 初始化 WiFi 热点
    ESP_LOGI(TAG, "Initializing WiFi AP...");
    err = wifi_init_softap();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi AP init failed: %s", esp_err_to_name(err));
        return;
    }
    xEventGroupSetBits(g_app_events, APP_EVT_WIFI_READY);

    // 3. 启动 HTTP 图像接收服务器
    ESP_LOGI(TAG, "Starting HTTP Server...");
    err = start_image_server();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP server start failed: %s", esp_err_to_name(err));
        return;
    }

    // Display/SD/LCD pipeline task (pin to core 1 to reduce contention with Wi-Fi stack)
    xTaskCreatePinnedToCore(photo_slideshow_task, "photo_slideshow", 8192, NULL, 5, NULL, 1);
}
