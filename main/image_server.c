#include "image_server.h"
#include <esp_http_server.h>
#include <esp_log.h>
#include <sys/param.h>
#include <errno.h>
#include <string.h>

static const char *TAG = "image_server";

static uint8_t current_photo_idx = 0; // 用于实现最多10张图片的轮换

static esp_err_t index_get_handler(httpd_req_t *req)
{
    httpd_resp_sendstr(req, "ESP Photo Frame is running. Send a POST request to /upload with image data.");
    return ESP_OK;
}

/* 处理接收图片的 POST 请求 */
static esp_err_t upload_image_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Receiving image data...");

    char filename[32];
    snprintf(filename, sizeof(filename), "/sdcard/photo_%d.rgb", current_photo_idx);

    FILE *f = fopen(filename, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file %s for writing. Error: %s", filename, strerror(errno));
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char buf[1024];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* 读取接收到的数据 */
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                // 如果超时，返回重试
                continue;
            }
            fclose(f);
            return ESP_FAIL;
        }

        fwrite(buf, 1, ret, f);
        remaining -= ret;
    }
    
    fclose(f);

    ESP_LOGI(TAG, "Image successfully saved as %s", filename);

    // 每次传完照片后，索引加 1，最多支持 10 张照片 (0~9)，如果存满则会覆盖最老的，形成循环。
    current_photo_idx = (current_photo_idx + 1) % 10;

    // 回复客户端接收成功
    httpd_resp_sendstr(req, "Image received and saved successfully");
    return ESP_OK;
}

esp_err_t start_image_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Starting HTTP server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t index_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = index_get_handler,
            .user_ctx  = NULL
        };
        esp_err_t err = httpd_register_uri_handler(server, &index_uri);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Register URI / failed: %s", esp_err_to_name(err));
        }

        // 注册 URI 处理程序
        httpd_uri_t image_upload_uri = {
            .uri       = "/upload",
            .method    = HTTP_POST,
            .handler   = upload_image_post_handler,
            .user_ctx  = NULL
        };
        err = httpd_register_uri_handler(server, &image_upload_uri);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Register URI failed: %s", esp_err_to_name(err));
            return err;
        }
        ESP_LOGI(TAG, "HTTP server started. Ready to receive images at /upload.");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Error starting server!");
    return ESP_FAIL;
}
