#pragma once

#include "driver/spi_master.h"

// Shared SPI Host
#define SHARED_SPI_HOST    SPI2_HOST

// SPI Bus Pins
#define PIN_NUM_MISO       12
#define PIN_NUM_MOSI       10
#define PIN_NUM_CLK        11

// LCD Pins
#define PIN_NUM_LCD_CS     5
#define PIN_NUM_LCD_DC     6
#define PIN_NUM_LCD_RST    7
#define LCD_H_RES          240
#define LCD_V_RES          320

// SD Card Pins
#define PIN_NUM_SD_CS      9

// Function to initialize the shared SPI bus
esp_err_t shared_spi_bus_init(void);
