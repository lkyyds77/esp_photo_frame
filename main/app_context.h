#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Global sync objects shared across modules
extern EventGroupHandle_t g_app_events;
extern SemaphoreHandle_t g_shared_bus_mutex;

// Event bits
#define APP_EVT_SD_READY        (1U << 0)
#define APP_EVT_LCD_READY       (1U << 1)
#define APP_EVT_WIFI_READY      (1U << 2)
#define APP_EVT_NEW_PHOTO       (1U << 3)

void app_context_init(void);

#ifdef __cplusplus
}
#endif
