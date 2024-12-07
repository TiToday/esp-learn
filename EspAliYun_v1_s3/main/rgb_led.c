#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "rgb_led.h"

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define EXAMPLE_RMT_TX_GPIO 8
#define EXAMPLE_STRIP_LED_NUMBER 1

static const char *TAG = "rgb_led";

led_strip_t *strip;  // LED灯带实例指针
rgb_led_t rgb_led;  // RGB LED状态

// 更新RGB状态
void rgb_led_update(void)
{
    ESP_LOGI(TAG, "rgb_led_update led_switch: %d, red: %d, green: %d, blue: %d", rgb_led.led_switch, rgb_led.red, rgb_led.green, rgb_led.blue);
    // 根据开关状态更新LED状态
    if (rgb_led.led_switch)
    {
        strip->set_pixel(strip, 0, rgb_led.red, rgb_led.green, rgb_led.blue);
    }
    else
    {
        strip->set_pixel(strip, 0, 0, 0, 0);
    }
    strip->refresh(strip, 50);
}

void rgb_led_init(void)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(EXAMPLE_RMT_TX_GPIO, RMT_TX_CHANNEL);
    // 将计数器时钟设置为40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // 安装ws2812驱动
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(EXAMPLE_STRIP_LED_NUMBER, (led_strip_dev_t)config.channel);
    // 创建LED灯带实例
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip)
    {
        ESP_LOGE(TAG, "install WS2812 driver failed");  // 安装WS2812驱动失败
    }
    // 清除LED灯带（关闭所有LED）
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    // 显示简单的彩虹追逐效果
    ESP_LOGI(TAG, "LED Rainbow Chase Start");
}
