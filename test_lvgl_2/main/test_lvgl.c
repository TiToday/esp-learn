#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lv_port.h"
#include "lv_demos.h"
#include "st7789_driver.h"
#include "ui_led.h"
#include "driver/gpio.h"
#include "ui_home.h"

#define TAG "ESP-EXAMPLE"
void app_main(void)
{
    e53_lcd_init();
    
    ui_home_create();

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
        vTaskDelay(pdMS_TO_TICKS(50));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }
}
