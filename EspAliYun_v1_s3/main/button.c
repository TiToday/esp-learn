#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "rgb_led.h"
#include "mqtt_solo.h"
#include "driver/gpio.h"

#define GPIO_INPUT_IO_0 9
#define GPIO_INPUT_PIN_SEL (1ULL << GPIO_INPUT_IO_0)
#define ESP_INTR_FLAG_DEFAULT 0

static const char *TAG = "button";
// static xQueueHandle gpio_evt_queue = NULL;

extern rgb_led_t rgb_led;

// 按键扫描函数
void read_button()
{
    // 如果按键被按下
    if (gpio_get_level(GPIO_INPUT_IO_0) == 0)
    {
        uint32_t tick1 = xTaskGetTickCount();
        uint32_t tick2 = xTaskGetTickCount();
        while (gpio_get_level(GPIO_INPUT_IO_0) == 0)
        {
            vTaskDelay(10 / portTICK_RATE_MS);

            // 检查是否长按超过100毫秒
            if (xTaskGetTickCount() > tick1 + 100)
            {
                tick1 = xTaskGetTickCount();
                ESP_LOGI(TAG, "按键长按\n");

                // 控制RGB LED的亮度增加
                rgb_led.red *= 1.1;
                rgb_led.green *= 1.1;
                rgb_led.blue *= 1.1;

                // 限制RGB LED的亮度最大值为255
                rgb_led.red = rgb_led.red > 255 ? 255 : rgb_led.red;
                rgb_led.green = rgb_led.green > 255 ? 255 : rgb_led.green;
                rgb_led.blue = rgb_led.blue > 255 ? 255 : rgb_led.blue;

                // 更新RGB LED的亮度
                rgb_led_update();
                // 执行MQTT消息发布操作
                // example_publish();
            }
        }

        // 检查是否短按（按下并释放时间不超过100毫秒）
        if (xTaskGetTickCount() > tick2 && xTaskGetTickCount() < tick2 + 100)
        {
            ESP_LOGI(TAG, "按键短按\n");

            // 切换RGB LED的开关状态
            if (rgb_led.led_switch)
            {
                rgb_led.led_switch = 0;
            }
            else
            {
                rgb_led.led_switch = 1;
                // 如果RGB LED的亮度为0，则设置默认亮度
                if (rgb_led.red == 0 && rgb_led.green == 0 && rgb_led.blue == 0)
                {
                    rgb_led.red = 100;
                    rgb_led.green = 100;
                    rgb_led.blue = 100;
                }
            }

            // 更新RGB LED的状态
            rgb_led_update();
            // 执行MQTT消息发布操作
            // example_publish();
        }
    }
}

// 按键扫描线程函数
static void gpio_task_example(void *arg)
{
    while (1)
    {
        // 每10毫秒检查一次按键状态
        vTaskDelay(10 / portTICK_RATE_MS);
        read_button();
    }
}

// 按键初始化函数
void button_init(void)
{
    // 配置GPIO
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    // 创建按键扫描线程
    xTaskCreate(gpio_task_example, "gpio_task_example", 4096, NULL, 10, NULL);
}
