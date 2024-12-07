#include "mqtt_aliot.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mbedtls/md5.h"
#include "mbedtls/md.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include <stdio.h>
#include "aliot_dm.h"
#include "button.h"

#define TAG "MQTT_ALIOT"
extern const char *g_aliot_ca;


static esp_mqtt_client_handle_t mqtt_handle = NULL;
static char s_is_mqtt_connected = 0;

char *get_clientid(void)
{
    uint8_t g_mac[6];
    static char client_id[32] = {0};

    esp_wifi_get_mac(WIFI_IF_STA, g_mac);
    snprintf(client_id, sizeof(client_id), "%02X%02X%02X%02X%02X%02X", g_mac[0], g_mac[1], g_mac[2], g_mac[3], g_mac[4], g_mac[5]);
    return client_id;
}

void mqtt_event_callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "MQTT_Aliot event callback");
    esp_mqtt_event_handle_t data = (esp_mqtt_event_handle_t)event_data;
    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_Aliot connected");
        s_is_mqtt_connected = 1;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_Aliot disconnected");
        s_is_mqtt_connected = 0;
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_Aliot published");

        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_Aliot subscribed");

        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_Aliot data received");
        ESP_LOGI(TAG, "Topic->%.*s",data->topic_len,data->topic);
        ESP_LOGI(TAG, "Payload->%.*s",data->data_len,data->data);
        break;
    default:
        break;
    }
}

void cal_md5(char *key, char *content, unsigned char *result)
{
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_MD5);

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, md_info, 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key, strlen(key));
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)content, strlen(content));
    mbedtls_md_hmac_finish(&ctx, result);
    mbedtls_md_free(&ctx);
}

void hex_to_str(uint8_t *input, uint32_t input_len, char *output)
{
    char *upper = "0123456789ABCDEF";
    int i = 0, j = 0;
    for (i = 0, j = 0; i < input_len; i++)
    {
        output[j++] = upper[(input[i] >> 4) & 0xF];
        output[j++] = upper[input[i] & 0xF];
    }
    output[j] = 0;
}

void mqtt_aliot_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {0};
    mqtt_cfg.broker.address.uri = ALIOT_MQTT_URL;
    mqtt_cfg.broker.address.port = 8883;

    char client_id[128];
    snprintf(client_id, sizeof(client_id), "%s|securemode=2,signmethod=hmacmd5|", get_clientid());
    char username[128];
    snprintf(username, sizeof(username), "%s&%s", ALIOT_DEVICENAME, ALIOT_PRODUCTKEY);
    char sign_content[256];
    snprintf(sign_content, sizeof(sign_content), "clientId%sdeviceName%sproductKey%s", get_clientid(), ALIOT_DEVICENAME, ALIOT_PRODUCTKEY);

    unsigned char password_hex[16];
    char password_str[33];
    cal_md5(ALIOT_DEVICESECRET, sign_content, password_hex);
    hex_to_str(password_hex, 16, password_str);

    mqtt_cfg.credentials.client_id = client_id;
    mqtt_cfg.credentials.username = username;
    mqtt_cfg.credentials.authentication.password = password_str;
    
    mqtt_cfg.broker.verification.certificate = g_aliot_ca;
    mqtt_handle = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(mqtt_handle, ESP_EVENT_ANY_ID, mqtt_event_callback, NULL);
    esp_mqtt_client_start(mqtt_handle);
}
void aliot_post_prorerty_int(const char *name, int value)
{
    // 创建物模型描述结构体
    ALIOT_DM_DES *dm = aliot_malloc_des();
    // 设置结构体属性值
    aliot_set_dm_int(dm,name,value);
    // 生成json字符串,保存在dm_js_str
    aliot_dm_serialize(dm);
    char topic[128];
    snprintf(topic,sizeof(topic),"/sys/%s/%s/thing/event/property/post",ALIOT_PRODUCTKEY,ALIOT_DEVICENAME);
    ESP_LOGW(TAG,"publish payload:%s",dm->dm_js_str);
    esp_mqtt_client_publish(mqtt_handle,topic,dm->dm_js_str,strlen(dm->dm_js_str),1,0);
    // 释放结构体
    aliot_dm_free(dm);
}
char is_mqtt_connected()
{
    return s_is_mqtt_connected;
}

const char *g_aliot_ca = "-----BEGIN CERTIFICATE-----\n"
                         "MIID3zCCAsegAwIBAgISfiX6mTa5RMUTGSC3rQhnestIMA0GCSqGSIb3DQEBCwUA"
                         "MHcxCzAJBgNVBAYTAkNOMREwDwYDVQQIDAhaaGVqaWFuZzERMA8GA1UEBwwISGFu"
                         "Z3pob3UxEzARBgNVBAoMCkFsaXl1biBJb1QxEDAOBgNVBAsMB1Jvb3QgQ0ExGzAZ"
                         "BgNVBAMMEkFsaXl1biBJb1QgUm9vdCBDQTAgFw0yMzA3MDQwNjM2NThaGA8yMDUz"
                         "MDcwNDA2MzY1OFowdzELMAkGA1UEBhMCQ04xETAPBgNVBAgMCFpoZWppYW5nMREw"
                         "DwYDVQQHDAhIYW5nemhvdTETMBEGA1UECgwKQWxpeXVuIElvVDEQMA4GA1UECwwH"
                         "Um9vdCBDQTEbMBkGA1UEAwwSQWxpeXVuIElvVCBSb290IENBMIIBIjANBgkqhkiG"
                         "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoK//6vc2oXhnvJD7BVhj6grj7PMlN2N4iNH4"
                         "GBmLmMdkF1z9eQLjksYc4Zid/FX67ypWFtdycOei5ec0X00m53Gvy4zLGBo2uKgi"
                         "T9IxMudmt95bORZbaph4VK82gPNU4ewbiI1q2loRZEHRdyPORTPpvNLHu8DrYBnY"
                         "Vg5feEYLLyhxg5M1UTrT/30RggHpaa0BYIPxwsKyylQ1OskOsyZQeOyPe8t8r2D4"
                         "RBpUGc5ix4j537HYTKSyK3Hv57R7w1NzKtXoOioDOm+YySsz9sTLFajZkUcQci4X"
                         "aedyEeguDLAIUKiYicJhRCZWljVlZActorTgjCY4zRajodThrQIDAQABo2MwYTAO"
                         "BgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUkWHoKi2h"
                         "DlS1/rYpcT/Ue+aKhP8wHwYDVR0jBBgwFoAUkWHoKi2hDlS1/rYpcT/Ue+aKhP8w"
                         "DQYJKoZIhvcNAQELBQADggEBADrrLcBY7gDXN8/0KHvPbGwMrEAJcnF9z4MBxRvt"
                         "rEoRxhlvRZzPi7w/868xbipwwnksZsn0QNIiAZ6XzbwvIFG01ONJET+OzDy6ZqUb"
                         "YmJI09EOe9/Hst8Fac2D14Oyw0+6KTqZW7WWrP2TAgv8/Uox2S05pCWNfJpRZxOv"
                         "Lr4DZmnXBJCMNMY/X7xpcjylq+uCj118PBobfH9Oo+iAJ4YyjOLmX3bflKIn1Oat"
                         "vdJBtXCj3phpfuf56VwKxoxEVR818GqPAHnz9oVvye4sQqBp/2ynrKFxZKUaJtk0"
                         "7UeVbtecwnQTrlcpWM7ACQC0OO0M9+uNjpKIbksv1s11xu0=\n"
                         "-----END CERTIFICATE-----";