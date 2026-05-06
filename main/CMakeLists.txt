idf_component_register(SRCS "lcd_display.c" "sd_card.c" "hardware_config.c" "main.c" "wifi_ap.c" "image_server.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi nvs_flash esp_http_server lwip esp_lcd fatfs)
