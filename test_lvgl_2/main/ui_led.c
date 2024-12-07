#include "lvgl.h"
#include "esp_log.h"
#include "driver/gpio.h"

static lv_obj_t *s_led_button = NULL;
static lv_obj_t *s_led_label = NULL;

void lv_led_button_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e); // 获取触发事件
    static uint32_t led_status = 0;
    switch (code)
    {
    case LV_EVENT_CLICKED:
        led_status = led_status ? 0 : 1;
        gpio_set_level(GPIO_NUM_8, led_status);
        break;

    default:
        break;
    }
}

void ui_led_create(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0); // 将当前屏幕的背景颜色设置为黑色，不覆盖之前的设置。

    s_led_button = lv_btn_create(lv_scr_act());                                     // 在当前激活的屏幕为父指针在屏幕上创建一个按钮
    lv_obj_align(s_led_button, LV_ALIGN_CENTER, 0, 0);                              // 将按钮对象s_led_button在界面中居中对齐
    lv_obj_set_style_bg_color(s_led_button, lv_palette_main(LV_PALETTE_ORANGE), 0); // lv_palette_main-->调色板函数
    lv_obj_set_size(s_led_button, 80, 40);

    s_led_label = lv_label_create(s_led_button); // 创建一个标签，用于显示LED的状态,可显示文字
    lv_obj_align(s_led_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(s_led_label, "LED"); // 设置标签的文本内容，暂时不能填中文

    lv_obj_set_style_text_font(s_led_label, &lv_font_montserrat_20, LV_STATE_DEFAULT);

    lv_obj_add_event_cb(s_led_button, lv_led_button_cb, LV_EVENT_CLICKED, NULL);
}