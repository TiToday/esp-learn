#include <stdio.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "string.h"

#define TAG "SmartConfig"
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
    }else if(event_base == SC_EVENT)
    {
        switch(event_id)
        {
            case SC_EVENT_SCAN_DONE:
                ESP_LOGI(TAG,"SCAN DONE");
                break;
            case SC_EVENT_GOT_SSID_PSWD:
                ESP_LOGI(TAG,"GOT SSID AND PASSWORD");
                smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
                wifi_config_t wifi_cfg = {0};
                memset(&wifi_cfg, 0, sizeof(wifi_cfg));
                snprintf((char*)wifi_cfg.sta.ssid, sizeof(wifi_cfg.sta.ssid), "%s", (char*)evt->ssid);
                snprintf((char*)wifi_cfg.sta.password, sizeof(wifi_cfg.sta.password), "%s", (char*)evt->password);
                ESP_LOGI(TAG,"SSID:%s",wifi_cfg.sta.ssid);
                ESP_LOGI(TAG,"PASSWORD:%s",wifi_cfg.sta.password);
                wifi_cfg.sta.bssid_set = evt->bssid_set;
                if (wifi_cfg.sta.bssid_set == true)
                {
                    memcpy(wifi_cfg.sta.bssid, evt->bssid, sizeof(wifi_cfg.sta.bssid));
                }
                esp_wifi_disconnect();
                esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
                esp_wifi_connect();
                break;
            case SC_EVENT_SEND_ACK_DONE:
                ESP_LOGI(TAG,"SEND ACK DONE");

                esp_smartconfig_stop();
                break;
            default:
                break;
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

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);



    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
    smartconfig_start_config_t sc_cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    esp_smartconfig_start(&sc_cfg);

    return;
}
