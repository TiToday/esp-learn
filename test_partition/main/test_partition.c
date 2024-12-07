#include <stdio.h>
#include "esp_partition.h"
#include "esp_log.h"
#include "string.h"

static const char *TAG = "partition";
static const esp_partition_t* partition_ptr = NULL;  //分区指针--->用于操作分区


#define USER_PARTITION_TYPE    0x40
#define USER_PARTITION_SUBTYPE 0x01


void app_main(void)
{
    partition_ptr = esp_partition_find_first(USER_PARTITION_TYPE, USER_PARTITION_SUBTYPE, NULL);

    if(partition_ptr == NULL)
    {
        ESP_LOGI(TAG, "partition not found");
        return;
    }

    esp_partition_erase_range(partition_ptr, 0, partition_ptr->size);
    const char *data = "hello world";
    esp_partition_write(partition_ptr, 0, data, strlen(data));

    char read_data[128];
    memset(read_data,0,sizeof(read_data));
    esp_partition_read(partition_ptr,0,read_data,strlen(data));

    ESP_LOGI(TAG, "read data: %s", read_data);

    return;
}
