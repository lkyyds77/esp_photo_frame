#include "sd_card.h"
#include "hardware_config.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "driver/gpio.h"

static const char *TAG = "SD_CARD";
static sdmmc_card_t *sd_card;

esp_err_t sd_card_init(void)
{
    esp_err_t ret = shared_spi_bus_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus.");
        return ret;
    }

    // 对于带有独立电平转换芯片的成名 TF 模块，最好取消其内部的上拉电阻，防止电压冲突
    gpio_set_pull_mode(PIN_NUM_MISO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_MOSI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_CLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_SD_CS, GPIO_PULLUP_ONLY);

    // 强行发几遍 dummy clock 唤醒卡
    // 注意：ESP-IDF底层会在发 CMD0 之前发 74 个以上的 dummy clock。
    
    // 强制把 LCD 的 CS 拉高，防止未初始化的 LCD 干扰 SPI 上的 SD 卡通信
    gpio_config_t lcd_cs_cfg = {
        .pin_bit_mask = (1ULL << PIN_NUM_LCD_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 1,
    };
    gpio_config(&lcd_cs_cfg);
    gpio_set_level(PIN_NUM_LCD_CS, 1);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 10,
        .allocation_unit_size = 16 * 1024
    };

    ESP_LOGI(TAG, "Initializing SD card via SPI");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SHARED_SPI_HOST;

    sdspi_device_config_t slot_config = {
        .gpio_cs = PIN_NUM_SD_CS,
        .gpio_cd = SDSPI_SLOT_NO_CD,
        .gpio_int = GPIO_NUM_NC,
        .gpio_wp = GPIO_NUM_NC,
        .host_id = host.slot,
    };

    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &sd_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card VFS FAT (error %s)", esp_err_to_name(ret));
        return ret;
    }

    sdmmc_card_print_info(stdout, sd_card);
    return ESP_OK;
}
