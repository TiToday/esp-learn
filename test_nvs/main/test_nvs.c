#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_log.h"

#define NAME_SPACE_WIFI1 "wifi1"
#define NAME_SPACE_WIFI2 "wifi2"

#define NVS_SSID_KEY "ssid"
#define NVS_PASSWORD_KEY "password"

void nvs_blob_read(const char *name_space, const char *key, void *value, int maxlen)
{
    nvs_handle_t nvs_handle;
    size_t length = 0;
    nvs_open(name_space, NVS_READONLY, &nvs_handle);
    nvs_get_blob(nvs_handle, key, NULL, &length); // 获取长度
    if (length && length < maxlen)
    {
        nvs_get_blob(nvs_handle, key, value, &length); // 读取值
    }
    nvs_close(nvs_handle);
}

void app_main(void)
{
    nvs_handle_t nvs_handle1;
    nvs_handle_t nvs_handle2;
    esp_err_t ret = nvs_flash_init();

    if (ret != ESP_OK)
    {
        nvs_flash_erase();
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // 命名空间1
    ESP_ERROR_CHECK(nvs_open(NAME_SPACE_WIFI1, NVS_READWRITE, &nvs_handle1));
    ESP_ERROR_CHECK(nvs_set_blob(nvs_handle1, NVS_SSID_KEY, "wifi_esp32-c6", strlen("wifi_esp32-c6")));
    ESP_ERROR_CHECK(nvs_set_blob(nvs_handle1, NVS_PASSWORD_KEY, "12345678", strlen("12345678")));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle1));
    nvs_close(nvs_handle1);

    // 命名空间2
    ESP_ERROR_CHECK(nvs_open(NAME_SPACE_WIFI2, NVS_READWRITE, &nvs_handle2));
    ESP_ERROR_CHECK(nvs_set_blob(nvs_handle2, NVS_SSID_KEY, "wifi_esp32", strlen("wifi_esp32")));
    ESP_ERROR_CHECK(nvs_set_blob(nvs_handle2, NVS_PASSWORD_KEY, "987654321", strlen("987654321")));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle2));
    nvs_close(nvs_handle2);

    vTaskDelay(pdMS_TO_TICKS(1000));


    char read_buffer[128];


    memset(read_buffer, 0, sizeof(read_buffer));
    nvs_blob_read(NAME_SPACE_WIFI1, NVS_SSID_KEY, read_buffer, sizeof(read_buffer));
    ESP_LOGI("NVS", "namespace:%s,key:%s --> value:%s", NAME_SPACE_WIFI1, NVS_SSID_KEY, read_buffer);

    memset(read_buffer, 0, sizeof(read_buffer));
    nvs_blob_read(NAME_SPACE_WIFI1, NVS_PASSWORD_KEY, read_buffer, sizeof(read_buffer));
    ESP_LOGI("NVS", "namespace:%s,key:%s --> value:%s", NAME_SPACE_WIFI1, NVS_PASSWORD_KEY, read_buffer);

    memset(read_buffer, 0, sizeof(read_buffer));
    nvs_blob_read(NAME_SPACE_WIFI2, NVS_SSID_KEY, read_buffer, sizeof(read_buffer));
    ESP_LOGE("NVS", "namespace:%s,key:%s --> value:%s", NAME_SPACE_WIFI2, NVS_SSID_KEY, read_buffer);

    memset(read_buffer, 0, sizeof(read_buffer));
    nvs_blob_read(NAME_SPACE_WIFI2, NVS_PASSWORD_KEY, read_buffer, sizeof(read_buffer));
    ESP_LOGE("NVS", "namespace:%s,key:%s --> value:%s", NAME_SPACE_WIFI2, NVS_PASSWORD_KEY, read_buffer);
}
