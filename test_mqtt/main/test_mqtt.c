#include <stdio.h>
#include "mqtt_client.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include "freertos/semphr.h"


#define TAG "MQTT"

#define MQTT_ADDRESS "mqtt://broker-cn.emqx.io"
#define MQTT_CLIENT_ID "MQTT_ESP32_20240830"
#define MQTT_USERNAME "admin"
#define MQTT_PASSWORD "public"

#define MQTT_TOPIC1 "/topic/a_esp32"    //ESP32往这个主题推送消息
#define MQTT_TOPIC2 "/topic/b_mqttx"    //mqttx往这个主题推送消息

#define WIFI_SSID "208"
#define WIFI_PASSWORD "iot208208208"


static esp_mqtt_client_handle_t mqtt_handle = NULL;

static SemaphoreHandle_t s_wifi_connected_sem = NULL;

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


void mqtt_event_callback (void* event_handler_arg,esp_event_base_t event_base,int32_t event_id,void* event_data)
{
    ESP_LOGI(TAG,"MQTT event callback");
    esp_mqtt_event_handle_t data = (esp_mqtt_event_handle_t)event_data;
    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG,"MQTT connected");
        esp_mqtt_client_subscribe_single(mqtt_handle,MQTT_TOPIC2,1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG,"MQTT disconnected");

        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG,"MQTT published");
        
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG,"MQTT subscribed");

        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG,"MQTT data received");
        ESP_LOGI(TAG,"Topic: %s",data->topic);
        ESP_LOGI(TAG,"Payload: %s",data->data);
        break;
    default:
        break;
    }

}

void mqtt_start(void)
{
    
    esp_mqtt_client_config_t mqtt_cfg = {0};
    mqtt_cfg.broker.address.uri = MQTT_ADDRESS;
    mqtt_cfg.broker.address.port = 1883;
    mqtt_cfg.credentials.client_id = MQTT_CLIENT_ID;
    mqtt_cfg.credentials.username = MQTT_USERNAME;
    mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;
    mqtt_handle = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(mqtt_handle, ESP_EVENT_ANY_ID, mqtt_event_callback, NULL);

    esp_mqtt_client_start(mqtt_handle);
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

    xSemaphoreTake(s_wifi_connected_sem, portMAX_DELAY);

    mqtt_start();

    int count = 0;
    while (1)
    {
        char count_str[32];
        snprintf(count_str, sizeof(count_str), "{\"count\":%d}", count);
        esp_mqtt_client_publish(mqtt_handle,MQTT_TOPIC1,count_str,strlen(count_str),1,0);
        count++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}
