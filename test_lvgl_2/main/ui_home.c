#include "lvgl.h"
#include "led_ws2812.h"
#include "dht11.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define WS2812_NUM 12

LV_IMG_DECLARE(temp_img);
LV_IMG_DECLARE(humidity_img);

static lv_obj_t *s_temp_image;
static lv_obj_t *s_humidity_image;

static lv_obj_t *s_temp_label;
static lv_obj_t *s_humidity_label;

static lv_obj_t *s_light_slider;

static lv_timer_t *s_dht11_timer;

ws2812_strip_handle_t ws2812_handle;

void light_slider_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e); // 获取触发事件
    switch (code)
    {
    case LV_EVENT_VALUE_CHANGED:
        lv_obj_t *slider_obj = lv_event_get_target(e);
        int32_t value = lv_slider_get_value(slider_obj);
        uint32_t rgb_value = 150 * value / 150;
        for (int led_index = 0; led_index < WS2812_NUM; led_index++)
        {
            ws2812_write(ws2812_handle, led_index, rgb_value, rgb_value, rgb_value);
        }

        break;
    default:
        break;
    }
}

void dht11_timer_cb(struct _lv_timer_t *t)
{
    int temp;
    int humidity;
    if (DHT11_StartGet(&temp, &humidity))
    {
        char disp_buf[32];
        snprintf(disp_buf, sizeof(disp_buf), "%.1f", (float)temp / 10.0);
        lv_label_set_text(s_temp_label, disp_buf);

        snprintf(disp_buf, sizeof(disp_buf), "%d%%", humidity);
        lv_label_set_text(s_humidity_label, disp_buf);
    }
}

void ui_home_create(void)
{
    // 创建调光用的进度条
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    s_light_slider = lv_slider_create(lv_scr_act());
    lv_obj_set_pos(s_light_slider, 60, 180);
    lv_obj_set_size(s_light_slider, 150, 15);
    lv_slider_set_range(s_light_slider, 0, 150);
    lv_obj_add_event_cb(s_light_slider, light_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // 创建温度图标
    s_temp_image = lv_img_create(lv_scr_act());
    lv_img_set_src(s_temp_image, &temp_img);
    lv_obj_set_pos(s_temp_image, 35, 35);

    // 创建湿度图标
    s_humidity_image = lv_img_create(lv_scr_act());
    lv_img_set_src(s_humidity_image, &humidity_img);
    lv_obj_set_pos(s_humidity_image, 35, 100);

    // 创建温度label
    s_temp_label = lv_label_create(lv_scr_act());
    lv_obj_set_pos(s_temp_label, 110, 40);

    lv_obj_set_style_text_font(s_temp_label, &lv_font_montserrat_38, 0);

    // 创建湿度label
    s_humidity_label = lv_label_create(lv_scr_act());
    lv_obj_set_pos(s_humidity_label, 110, 110);

    lv_obj_set_style_text_font(s_humidity_label, &lv_font_montserrat_38, 0);

    // 创建定时器
    s_dht11_timer = lv_timer_create(dht11_timer_cb, 2000, NULL);

    // 初始化ws2812
    ws2812_init(GPIO_NUM_8, WS2812_NUM, &ws2812_handle);

    // 初始化dht11

    DHT11_Init(GPIO_NUM_21);
}
