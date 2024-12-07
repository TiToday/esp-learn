#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "nvs_storage.h"
#include "esp_wifi.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_err.h"
#include "cJSON.h"
#include "esp_log.h"
#include "mqtt_solo.h"

extern control_t control;
extern user_t user;
void nvs_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

esp_err_t save_config(const char *key, const char *value)
{
    nvs_handle_t my_handle;
    esp_err_t err;
    char storage_namespace[] = "config";

    // Open the NVS namespace
    err = nvs_open(storage_namespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        return err;

    // Set the key-value pair
    err = nvs_set_str(my_handle, key, value);
    if (err != ESP_OK)
        return err;

    // Commit the changes
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
        return err;

    // Close the NVS handle
    nvs_close(my_handle);
    print_config(key);

    return ESP_OK;
}

esp_err_t print_config(const char *key)
{
    nvs_handle_t my_handle;
    esp_err_t err;
    char storage_namespace[] = "config";

    // Open the NVS namespace
    err = nvs_open(storage_namespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        return err;

    // Read the value associated with the key
    char value[20];
    size_t length = sizeof(value);
    err = nvs_get_str(my_handle, key, value, &length);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;

    printf("this is printf config\n");
    printf("%s = %s\n", key, value);

    // Close the NVS handle
    nvs_close(my_handle);
    return ESP_OK;
}

void socket_task(void *pvParameters)
{
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // 创建 socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("Failed to create socket!\n");
        vTaskDelete(NULL);
    }

    // 设置服务器地址和端口
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0)
    {
        printf("Invalid IP address!\n");
        close(sock);
        vTaskDelete(NULL);
    }

    // 连接服务器
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Failed to connect to server!\n");
        perror("connect");
        close(sock);
        vTaskDelete(NULL);
    }
    printf("Connected to the server\n");

    // 准备 GET 请求
    char request[128];
    char request_format;
    switch (control.flag)
    {
    case 1:
        request_format = "GET %s?face_id=%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
        // 格式化socket请求
        snprintf(request, sizeof(request), request_format, REQUEST_URL_1, control.face_ID, SERVER_IP);
        // 发送网络请求
        break;
    case 2:
        request_format = "GET %s?QRcode=%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
        // cJSON解析，获取到对应的二维码返回信息
        snprintf(request, sizeof(request), request_format, REQUEST_URL_2, control.scan_QR_code, SERVER_IP);
        break;
    case 3:
        request_format = "GET %s?regulation=%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
        // cJSON解析，获取到对应的规则模式
        snprintf(request, sizeof(request), request_format, REQUEST_URL_3, control.regulation, SERVER_IP);
        break;
    case 4:
        request_format = "GET %s?gpt_id=%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
        // cJSON解析，获取生长周期ID
        snprintf(request, sizeof(request), request_format, REQUEST_URL_4, control.GPT_ID, SERVER_IP);
        break;
    default:
        request_format = "GET %s?account=%s&password=%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
        snprintf(request, sizeof(request), request_format, REQUEST_URL_5, user.account, user.password, SERVER_IP);
        break;
    }
    // 发送 GET 请求到服务器
    if (send(sock, request, strlen(request), 0) < 0)
    {
        printf("Failed to send GET request!\n");
        perror("send");
        close(sock);
        vTaskDelete(NULL);
    }
    printf("GET request sent\n");

    // 接收并处理服务器响应
    char rx_buffer[4096];
    memset(rx_buffer, 0, sizeof(rx_buffer));
    int len;
    while ((len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0)) > 0)
    {
        // Find the start of the JSON string
        char *json_start = strstr(rx_buffer, "{");
        if (json_start != NULL)
        {
            cJSON *root = cJSON_Parse(json_start);
            if (root == NULL)
            {
                printf("Failed to parse JSON!\n");
            }
            else
            {
                cJSON *valueJson = cJSON_GetObjectItem(root, "value");
                if (valueJson == NULL)
                {
                    printf("Failed to get 'value' from JSON!\n");
                }
                else
                {
                    const char *value = valueJson->valuestring;

                    switch (control.flag)
                    {
                    case 1:
                        // cJSON获取用户个人信息
                        // 页面跳转
                        break;
                    case 2:
                        // 收到200就显示成功
                        break;
                    case 3:
                        // 获取到对应模式，直接进行设置，并显示切换成功
                        break;
                    case 4:
                        // 获取对应资料，在页面上进行显示
                        break;
                    default:
                        // 收到200就显示成功
                        break;
                    }
                    // Save value to NVS
                    // esp_err_t save_result = save_config("value", value);
                    // if (save_result == ESP_OK)
                    // {
                    //     printf("Value saved to NVS: %s\n", value);
                    // }
                    // else
                    // {
                    //     printf("Failed to save value to NVS!\n");
                    // }
                }
                cJSON_Delete(root);
            }
            break;
        }
        memset(rx_buffer, 0, sizeof(rx_buffer));
    }

    if (len < 0)
    {
        printf("Error reading from socket!\n");
        perror("recv");
    }

    // 关闭 socket
    close(sock);

    vTaskDelete(NULL);
}
