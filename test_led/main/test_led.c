#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_PIN GPIO_NUM_8

void Led_run_task(void *param)
{
    int GPIO_Level = 0;
    while(1)
    {
        GPIO_Level = GPIO_Level ? 0 : 1;
        gpio_set_level(LED_PIN,GPIO_Level);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    gpio_config_t Led_cfg = {
        .intr_type = GPIO_INTR_DISABLE,         //禁止中断
        .mode = GPIO_MODE_OUTPUT,               //设置为输出模式
        .pin_bit_mask = (1 << LED_PIN),         //指定GPIO
        .pull_down_en = GPIO_PULLDOWN_DISABLE,  //禁止下拉
        .pull_up_en = GPIO_PULLUP_DISABLE,      //禁止上拉
    };
    gpio_config(&Led_cfg);

    xTaskCreatePinnedToCore(Led_run_task,"LED",2048,NULL,3,NULL,0);
}