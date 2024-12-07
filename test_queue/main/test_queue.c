#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>


QueueHandle_t queue_Handle = NULL;

typedef struct
{
    int value;
}queue_data_t;

void tastA(void* param)
{
    queue_data_t data;
    while(1)
    {
        if(pdTRUE == xQueueReceive(queue_Handle,&data,100))
        {
            ESP_LOGI("queue","Recieve queue value:%d",data.value);
        }
    }
}

void tastB(void* param)
{
    queue_data_t data;
    memset(&data,0,sizeof(queue_data_t));
    while(1)
    {
        xQueueSend(queue_Handle,&data,100);
        vTaskDelay(pdMS_TO_TICKS(1000));
        data.value++;
    }
}

void app_main(void)
{
    queue_Handle = xQueueCreate(10,sizeof(queue_data_t));
    xTaskCreatePinnedToCore(tastA,"taskA",2048,NULL,3,NULL,0);
    xTaskCreatePinnedToCore(tastB,"taskB",2048,NULL,3,NULL,0);
}
