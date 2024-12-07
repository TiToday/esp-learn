#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_err.h"
#include <time.h>
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_sntp.h"


#define TAG "WIFI_STA"
static const char *TAG1 = "SNTP Example";
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
            break;
        default:
            break;
        }
    }
}
static void obtain_time(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_init();
    esp_netif_create_default_wifi_sta();

    // Configure SNTP with Aliyun NTP server
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    esp_sntp_set_time_sync_notification_cb(NULL); // No callback needed

    // Connect to Wi-Fi (ensure you implement this)
    // example_connect();

    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;
    
    while (retry < retry_count) {
        if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            break; // Time synchronized
        }
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry + 1, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        retry++;
    }

    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_LOGI(TAG, "Current timestamp: %lld", now);

    // Stop SNTP
    esp_sntp_stop();
}


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta.threshold.authmode = WIFI_AUTH_WPA2_PSK,
        .sta.pmf_cfg.capable = true,
        .sta.pmf_cfg.required = false,
        .sta = {
            .ssid = "208",
            .password = "iot208208208",
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());


    obtain_time();

    return;
}
