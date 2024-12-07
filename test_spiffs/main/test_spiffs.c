#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_err.h"

void app_main(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true,
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE("SPIFFS", "Failed to mount or format filesystem");
        return;
    }

    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK)
    {
        ESP_LOGE("SPIFFS", "Check failed");
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE("SPIFFS", "Get Spiffs info failed");
        return;
    }

    ESP_LOGI("SPIFFS", "Total: %d, Used: %d", total, used);

    if (used > total)
    {
        ret = esp_spiffs_check(conf.partition_label);
        if (ret != ESP_OK)
        {
            ESP_LOGE("SPIFFS", "Used > Total ---> Check failed");
            return;
        }
    }


    FILE *f = fopen("/spiffs/hello.txt", "w");
    if(f == NULL)
    {
        ESP_LOGE("SPIFFS", "Failed to open file for writing");
        return;
    }
    fprintf(f, "Hello World!\n");
    fclose(f);

    vTaskDelay(pdMS_TO_TICKS(2000));

    f = fopen("/spiffs/hello.txt", "r");
    if (f == NULL)
    {
        ESP_LOGE("SPIFFS", "Failed to open file for reading");
        return;
    }

    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);

    char *pos = strchr(line, '\n');
    if (pos)
    {
        *pos = 0;
    }
    ESP_LOGI("SPIFFS", "Read from file: '%s'", line);

    esp_vfs_spiffs_unregister(conf.partition_label);
    


}
