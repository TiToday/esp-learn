#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "ui_mjpeg_player.h"
#include "lvgl.h"
#include "lv_port.h"
#include "st7789_driver.h"
#include "sdcard_file.h"

#define TAG "main"

void lvgl_task(void *param)
{
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
}

void app_main(void)
{
    spiffs_init("ui_img","/img");
    e53_lcd_init();
    ui_mjpeg_create();
    xTaskCreatePinnedToCore(lvgl_task, "lvgl_task", 4096, NULL, 5, NULL, 0);


}
