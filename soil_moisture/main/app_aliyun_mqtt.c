#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/adc.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"
#include "aliyun_mqtt.h"
#include "cJSON.h"

#define ADC_CHANNEL GPIO_NUM_0  // IO1 引脚
#define ADC_WIDTH   ADC_WIDTH_BIT_DEFAULT
#define ADC_ATTEN   ADC_ATTEN_DB_12

char local_data_buffer[1024] = {0};
char mqtt_publish_data1[] = "mqtt connect ok ";
char mqtt_publish_data2[] = "mqtt subscribe successful";
char mqtt_publish_data3[] = "mqtt i am esp32";
char pub_payload[512];
static const char *TAG = "APP_ALIYUN_MQTT";
  float app_soil_moisture(void) {
    float soil_moisture = 0.00;
    // 配置 ADC 宽度
    esp_err_t ret = adc1_config_width(ADC_WIDTH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC width: %s", esp_err_to_name(ret));
        return 0.00;
    }

    // 配置 ADC 通道的衰减
    ret = adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC attenuation: %s", esp_err_to_name(ret));
        return 0.00;
    }
        // 读取 ADC 值
        int adc_reading = adc1_get_raw(ADC_CHANNEL);
        if (adc_reading < 0) {
            ESP_LOGE(TAG, "ADC reading failed");
        } else {
            // 打印 ADC 值
            ESP_LOGI(TAG, "ADC Reading: %d", adc_reading);

            // 转换为电压值（3.3V）
            float voltage = adc_reading * (3.3 / 4095.0);
            ESP_LOGI(TAG, "Voltage: %.2f V", voltage);

            // 转换为土壤湿度值
            // 假设 0V 对应 0% 湿度，3.3V 对应 100% 湿度
            soil_moisture = ((3.3-voltage) / 3.3) * 100;
            ESP_LOGI(TAG, "Soil Moisture: %.2f%%", soil_moisture);

        }
        return soil_moisture;
}


esp_mqtt_client_handle_t client;
static esp_err_t app_mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{

    client = event->client;
    int msg_id;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, CONFIG_AliYun_PUBLISH_TOPIC_USER_UPDATE, mqtt_publish_data1, strlen(mqtt_publish_data1), 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_subscribe(client, CONFIG_AliYun_SUBSCRIBE_TOPIC_USER_GET, 0);
        // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // 注意：这里使用了同样的发布主题，根据实际逻辑可能需要调整
        // msg_id = esp_mqtt_client_publish(client, CONFIG_AliYun_PUBLISH_TOPIC_USER_UPDATE, mqtt_publish_data2, strlen(mqtt_publish_data2), 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);

        break;
    case MQTT_EVENT_DATA:
        ESP_LOGE(TAG, "MQTT_EVENT_DATA");
        printf("DATA=%.*s\r\n",event->data_len, event->data);
        // 处理接收到的数据逻辑
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}
static void app_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    app_mqtt_event_handler_cb(event_data);
}

void app_aliyun_mqtt_init(void)
{
    aliyun_mqtt_init(app_mqtt_event_handler);
    while (1)
    {
        float soil_moisture = app_soil_moisture();
        sprintf(pub_payload,"{\"id\": \"123\", \"version\": \"1.0\", \"sys\": {\"ack\": 1}, \"params\": {\"soilHumidity\":%.2f }, \"method\": \"thing.event.property.post\"}",soil_moisture);
        int msg_id = esp_mqtt_client_publish(client, CONFIG_AliYun_PUBLISH_TOPIC_USER_POST, pub_payload, strlen(pub_payload), 2, 0);
        ESP_LOGI(TAG, "msg=%d", msg_id);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}