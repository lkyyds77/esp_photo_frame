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

static const char *TAG = "main";

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

    // Initialize SD card
    ESP_LOGI(TAG, "Initializing SD Card...");
    sd_card_init();

    // Initialize LCD
    ESP_LOGI(TAG, "Initializing LCD...");
    lcd_display_init();

    // 2. 初始化 WiFi 热点
    ESP_LOGI(TAG, "Initializing WiFi AP...");
    wifi_init_softap();

    // 3. 启动 HTTP 图像接收服务器
    ESP_LOGI(TAG, "Starting HTTP Server...");
    start_image_server();

    // 4. Main Loop: Read /sdcard/photo.rgb and render to LCD
    uint8_t *img_buf = malloc(LCD_H_RES * LCD_V_RES * 2); // Assuming roughly RGB565 memory size for a frame
    if (!img_buf) {
        ESP_LOGE(TAG, "Failed to allocate memory for image buffer");
        return;
    }

    while (1) {
        // ... (LCD render logic) ... 
        bool photo_found = false;

        // 遍历这10个名字的图片 photo_0.rgb 到 photo_9.rgb
        for (int i = 0; i < 10; i++) {
            char filename[32];
            snprintf(filename, sizeof(filename), "/sdcard/photo_%d.rgb", i);

            FILE *f = fopen(filename, "rb");
            if (f) {
                photo_found = true;
                size_t expected_size = LCD_H_RES * LCD_V_RES * 2;
                size_t bytes_read = fread(img_buf, 1, expected_size, f);
                fclose(f); // 立刻关闭文件释放死锁，不要让文件句柄横跨后面的屏幕绘制时间
                
                if (bytes_read > 0) {
                    if (bytes_read < expected_size) {
                        memset(img_buf + bytes_read, 0, expected_size - bytes_read);
                    }

                    // 转换大小端 (Endianness swap)
                    uint16_t *pixels = (uint16_t *)img_buf;
                    for (int j = 0; j < LCD_H_RES * LCD_V_RES; j++) {
                        pixels[j] = (pixels[j] >> 8) | (pixels[j] << 8);
                    }

                    lcd_draw_bitmap(0, 0, LCD_H_RES, LCD_V_RES, pixels);
                }
                
                // 每张照片显示5秒钟
                vTaskDelay(pdMS_TO_TICKS(5000));
            }
        }
        
        // 如果一张照片也没有找到，休息1秒钟再试，避免死循环触发看门狗导致无限重启
        if (!photo_found) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}
