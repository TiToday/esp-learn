#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "dirent.h"

#define MOUNT_POINT "/spiffs"
#define TAG "SPIFFS"

esp_err_t spiffs_init(char *partition_label, char *mount_point)
{
    esp_vfs_spiffs_conf_t conf = {
      .base_path = mount_point,   //可以认为挂着点，后续使用C库函数fopen("/spiffs/...")
      .partition_label = partition_label,  //指定spiffs分区，如果为NULL，则默认为分区表中第一个spiffs类型的分区
      .max_files = 10,           //最大可同时打开的文件数
      .format_if_mount_failed = false,
    };

    //初始化和挂载spiffs分区
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    ESP_LOGI(TAG, "Initializing SPIFFS");

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. ");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the filesystem (%s). ", esp_err_to_name(ret));
        }
        return ESP_OK;
    }
    ESP_LOGI(TAG, "Filesystem mounted");
    return ESP_OK;


}

int spiffs_filelist(const char (**file)[256])
{

    DIR *dir;
    struct dirent *entry;
    static char filename[20][256] = {0};

    // 打开目录
    dir = opendir(MOUNT_POINT);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory '%s'", MOUNT_POINT);
        return 0;
    }

    // 读取目录中的文件列表
    int file_cnt = 0;
    while ((entry = readdir(dir)) != NULL) {
        
        printf("%s\n", entry->d_name);
        snprintf(&filename[file_cnt][0],256,"%s",entry->d_name);
        file_cnt++;
        if(file_cnt >= 20)
            break;
    }
    closedir(dir);
    *file = filename;
    return file_cnt;
}

