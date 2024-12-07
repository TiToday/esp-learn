#include <stdio.h>
#include "mqtt_client.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include "freertos/semphr.h"
#include "mqtt_aliot.h"
#include "driver/gpio.h"
#include "button.h"
#include "aliot_dm.h"

#define TAG "MQTT"
#define SHOUT_EV (BIT0)
#define WIFI_SSID "208"
#define WIFI_PASSWORD "iot208208208"

static esp_mqtt_client_handle_t mqtt_handle = NULL;

static SemaphoreHandle_t s_wifi_connected_sem = NULL;

static EventGroupHandle_t s_button_event = NULL;

void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "WIFI_EVENT_STA_DISCONNECTED");
            ESP_LOGW(TAG, "WAIT 5 SECONDS,TRY TO CONNECT AGAIN");
            vTaskDelay(pdMS_TO_TICKS(5000));

            esp_wifi_connect();
            break;
        default:
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
            xSemaphoreGive(s_wifi_connected_sem);
            break;
        default:
            break;
        }
    }
}

void btn_short_press(void)
{
    xEventGroupSetBits(s_button_event, SHOUT_EV);
}

// 短按回调函数
void button_task(void *param)
{
    // 按键配置结构体
    button_config_t button_cfg =
        {
            .gpio_num = GPIO_NUM_35,     // gpio号
            .active_level = 0,           // 按下的电平
            .long_press_time = 2000,     // 长按时间
            .short_cb = btn_short_press, // 短按回调函数
            .long_cb = NULL,             // 长按回调函数
        };

    s_button_event = xEventGroupCreate();

    /** 设置按键事件
     * @param cfg   配置结构体
     * @return ESP_OK or ESP_FAIL
     */
    button_event_set(&button_cfg);

    gpio_config_t led_gpio = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ull << GPIO_NUM_27),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&led_gpio);

    EventBits_t ev;
    int led_level = 0;
    while (1)
    {
        ev = xEventGroupWaitBits(s_button_event, SHOUT_EV, pdTRUE, pdFALSE, pdMS_TO_TICKS(5000));
        if (ev & SHOUT_EV)
        {
            if (led_level)
            {
                led_level = 0;
            }
            else
            {
                led_level = 1;
            }
            gpio_set_level(GPIO_NUM_27, led_level);

            if (is_mqtt_connected())
            {
                aliot_post_prorerty_int("LightSwitch", led_level);
            }
        }
    }
}

void app_main(void)
{

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    s_wifi_connected_sem = xSemaphoreCreateBinary();

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta.threshold.authmode = WIFI_AUTH_WPA2_PSK,
        .sta.pmf_cfg.capable = true,
        .sta.pmf_cfg.required = false,
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    xTaskCreatePinnedToCore(button_task, "button", 4096, NULL, 2, NULL, 1);

    xSemaphoreTake(s_wifi_connected_sem, portMAX_DELAY);

    mqtt_aliot_init();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
