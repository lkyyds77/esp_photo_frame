#include "app_context.h"

#include "esp_check.h"

EventGroupHandle_t g_app_events = NULL;
SemaphoreHandle_t g_shared_bus_mutex = NULL;

void app_context_init(void)
{
    if (!g_app_events) {
        g_app_events = xEventGroupCreate();
    }
    if (!g_shared_bus_mutex) {
        g_shared_bus_mutex = xSemaphoreCreateMutex();
    }
}
