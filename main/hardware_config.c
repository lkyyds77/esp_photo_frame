#include "hardware_config.h"

static bool spi_bus_initialized = false;

esp_err_t shared_spi_bus_init(void)
{
    if (spi_bus_initialized) {
        return ESP_OK;
    }

    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 400000,
    };

    esp_err_t ret = spi_bus_initialize(SHARED_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret == ESP_OK) {
        spi_bus_initialized = true;
    }
    return ret;
}
