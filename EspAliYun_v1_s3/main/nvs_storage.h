#ifndef _NVS_STORAGE_H__
#define _NVS_STORAGE_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/task.h"
#include "esp_err.h"
#include "cJSON.h"

#define SOCKET_TAG "Entry Socket Task"
#define SERVER_IP "10.50.47.69" 
//"10.50.47.69"//"47.113.151.204"
#define SERVER_PORT 8080
#define REQUEST_URL_1 "/login"
#define REQUEST_URL_2 "/QRcode"
#define REQUEST_URL_3 "/regulation"
#define REQUEST_URL_4 "/gpt"
#define REQUEST_URL_5 "/login_account"
TaskHandle_t network_connected_task;
void nvs_init(void);
esp_err_t save_config(const char *key, const char *value);
esp_err_t print_config(const char *key);
void socket_task(void *pvParameters);


#endif
