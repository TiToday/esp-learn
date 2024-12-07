#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lv_port.h"
#include "lv_demos.h"
#include "st7789_driver.h"
#include "ui_led.h"
#include "driver/gpio.h"
#include "esp_log.h"


#define TAG "ESP-EXAMPLE"

void app_main(void)
{

    gpio_config_t led_gpio = {
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL<<GPIO_NUM_8),
    };
    gpio_config(&led_gpio);
    gpio_set_level(GPIO_NUM_8, 0);

    e53_lcd_init();
    ui_led_create();


    // /* Turn on display backlight */
    //  bsp_display_backlight_on();
    
    ESP_LOGI(TAG, "Example initialization done.");

    
    // raise the task priority of LVGL and/or reduce the handler period can improve the performance
    vTaskDelay(pdMS_TO_TICKS(10));
    // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
    lv_timer_handler();

    while (1)
    {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }

    // while(1)
    // {
    //     lv_task_handler();
    //     vTaskDelay(pdTICKS_TO_MS(10));
    // }
}
