#ifndef _MQTT_SOLO_H__
#define _MQTT_SOLO_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mqtt_api.h"
#include "dm_wrapper.h"
#include "cJSON.h"

typedef struct
{
    int flag;// 事件ID，人脸识别1，二维码扫描2，规则3，助手4
    int face_detection; // 人脸识别开关
    int face_ID;        // 人脸ID
    int scan_QR_code;   // 扫描二维码开关
    int QR_content;     // 二维码内容
    int regulation;      // 规则
    int GPT_ID;          // 助手ID
} control_t;

typedef struct{
    char account[20];
    char password[20];
}user_t;

typedef struct{
    int device1;
    int device2;
    int device3;
    int device4;
}ex_device_t;

ex_device_t device;
user_t user;
control_t control;
int example_publish(void);
void mqtt_main(void * pvParameters);
int gate_publish(void);
void device_task(void *pvParams);
// void closeDevice_task(void *pvParams);
#endif
