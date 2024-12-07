#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "main";

//二进制信号量
static SemaphoreHandle_t s_testBinSem;
//计数信号量
static SemaphoreHandle_t s_testCountSem;
//互斥信号量
static SemaphoreHandle_t s_testMuxSem;

void sem_taskA(void* param)
{
    const int count_sem_num = 5;
    while(1)
    {
        //向二进制信号量释放信号
        xSemaphoreGive(s_testBinSem);

        //向计数信号量释放5个信号
        for(int i = 0;i < count_sem_num;i++)
        {
            xSemaphoreGive(s_testCountSem);
        }

        //向互斥信号量释放信号
        xSemaphoreGiveRecursive(s_testMuxSem);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void sem_taskB(void* param)
{
    BaseType_t ret = 0;
    while(1)
    {
        //无限等待二进制信号量，直到获取成功才返回
        ret = xSemaphoreTake(s_testBinSem,portMAX_DELAY);
        if(ret == pdTRUE)
            ESP_LOGI(TAG,"take binary semaphore");


        //接收计数信号量，每次接收200ms，直到接收失败才结束循环
        int sem_count = 0;
        do
        {
            ret = xSemaphoreTake(s_testCountSem,pdMS_TO_TICKS(200));
            if(ret==pdTRUE)
            {
                ESP_LOGW(TAG,"take count semaphore,count:%d\r\n",++sem_count);
            }
        }while(ret ==pdTRUE);
        
        //无限等待互斥信号量，直到获取成功才返回，这里用法和二进制信号量极为类似
        ret = xSemaphoreTakeRecursive(s_testMuxSem,portMAX_DELAY);
        if(ret == pdTRUE)
            ESP_LOGE(TAG,"take Mutex semaphore");

    }
}

void app_main(void)
{
    s_testBinSem = xSemaphoreCreateBinary();
    s_testCountSem = xSemaphoreCreateCounting(5,0);
    s_testMuxSem = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(sem_taskA,"sem_taskA",2048,NULL,3,NULL,0);
    xTaskCreatePinnedToCore(sem_taskB,"sem_taskB",2048,NULL,3,NULL,0);
}
