#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_log.h"

static TaskHandle_t taskA_handle;
static TaskHandle_t taskB_handle;

void taskA(void *param)
{
    uint32_t value = 0;
     vTaskDelay(pdMS_TO_TICKS(200));
    while(1)
    {
        xTaskNotify(taskB_handle,value,eSetValueWithoutOverwrite);
        vTaskDelay(pdMS_TO_TICKS(1000));
        value++;
    }
}

void taskB(void *param)
{
    uint32_t value;
    while(1)
    {
       xTaskNotifyWait(0x00,ULONG_LONG_MAX,&value,portMAX_DELAY);
       ESP_LOGI("ev","notify wait value:%lu",value);
    }
}

void app_main(void)
{
    
    xTaskCreatePinnedToCore(taskA,"taskA",2048,NULL,3,&taskA_handle,0);
    xTaskCreatePinnedToCore(taskB,"taskB",2048,NULL,3,&taskB_handle,0);
}
