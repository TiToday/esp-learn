#include "mqtt_solo.h"
#include <string.h>
#include "rgb_led.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_storage.h"

char DEMO_PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1] = {0};
char DEMO_DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1] = {0};
char DEMO_DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1] = {0};

extern rgb_led_t rgb_led;

extern bool rx_sta_bool;

extern ex_device_t device;

extern bool flag1;
environment_variable_t environment_variable;

#define EXAMPLE_TRACE(fmt, ...)                        \
    do                                                 \
    {                                                  \
        HAL_Printf("%s|%03d :: ", __func__, __LINE__); \
        HAL_Printf(fmt, ##__VA_ARGS__);                \
        HAL_Printf("%s", "\r\n");                      \
    } while (0)

// 云端消息到达回调函数
void example_message_arrive(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    // 解析云端数据
    cJSON *root = NULL;
    cJSON *params = NULL;
    cJSON *rgb_color = NULL;

    // 获取消息
    iotx_mqtt_topic_info_t *topic_info = (iotx_mqtt_topic_info_pt)msg->msg;

    // 判断消息事件类型
    switch (msg->event_type)
    {
    case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
        // 打印接收到的消息
        EXAMPLE_TRACE("Message Arrived:");
        EXAMPLE_TRACE("Topic  : %.*s", topic_info->topic_len, topic_info->ptopic);
        EXAMPLE_TRACE("Payload: %.*s", topic_info->payload_len, topic_info->payload);
        EXAMPLE_TRACE("\n");
        char temp_topic[256];

        snprintf(temp_topic, sizeof(temp_topic), "%.*s", topic_info->topic_len, topic_info->ptopic);

        root = cJSON_Parse(topic_info->payload);
        char *pstr;
        if (strcmp(temp_topic, "/a1BC3i5kPNZ/FP_box_001/user/getMsg") == 0)
        {
            cJSON *event = NULL;

            pstr = cJSON_PrintUnformatted(root); // 无格式的CJson
            printf("%s\n\n", pstr);

            event = cJSON_GetObjectItem(root, "flag");
            pstr = cJSON_PrintUnformatted(event);
            printf("event: %s\n\n", pstr);

            int flag = atoi(pstr);
            printf("event: %d\n\n", flag);

            // cJSON *item;
            switch (flag)
            {
            case 1:
                // cJSON解析，获取到对应的face_id
                //  item=cJSON_GetObjectItem(root,"face_detection");
                control.face_ID = cJSON_GetObjectItem(root, "face_detection")->valueint;
                // 发送网络请求
                xTaskCreate(socket_task, "socket_task", 8192, &flag, 5, &network_connected_task);
                break;
            case 2:
                // cJSON解析，获取到对应的二维码返回信息
                //  item=cJSON_GetObjectItem(root,"scan_QR_code");
                control.scan_QR_code = cJSON_GetObjectItem(root, "scan_QR_code")->valueint;
                // 网络请求发送给服务器
                xTaskCreate(socket_task, "socket_task", 8192, &flag, 5, &network_connected_task);
                break;
            case 3:
                // cJSON解析，获取到对应的规则模式
                //  item=cJSON_GetObjectItem(root,"regulation");
                control.regulation = cJSON_GetObjectItem(root, "regulation")->valueint;
                // 网络请求，获取规则
                xTaskCreate(socket_task, "socket_task", 8192, &flag, 5, &network_connected_task);
                break;
            case 4:
                // cJSON解析，获取生长周期ID
                //  item=cJSON_GetObjectItem(root,"GPT_ID");
                control.GPT_ID = cJSON_GetObjectItem(root, "GPT_ID")->valueint;
                // 网络请求服务器，获取对应的资料
                xTaskCreate(socket_task, "socket_task", 8192, &flag, 5, &network_connected_task);
                break;
            default:
                // 密码登陆注册
                xTaskCreate(socket_task, "socket_task", 8192, &flag, 5, &network_connected_task);
                break;
            }

            free(pstr);
        }
        else if (strcmp(temp_topic, "/sys/a1BC3i5kPNZ/FP_box_001/thing/service/property/set") == 0)
        {
            cJSON *params = NULL;
            cJSON *device_temp = NULL;

            pstr = cJSON_PrintUnformatted(root); // 无格式的CJson
            printf("%s\n\n", pstr);

            params = cJSON_GetObjectItem(root, "params");
            pstr = cJSON_PrintUnformatted(params);
            printf("params: %s\n\n", pstr);

            device_temp = cJSON_GetObjectItem(params, "ex_device_ports");
            pstr = cJSON_PrintUnformatted(device_temp);
            printf("device_temp: %s\n\n", pstr);

            device.device1 = cJSON_GetObjectItem(device_temp, "device_port1")->valueint;
            device.device2 = cJSON_GetObjectItem(device_temp, "device_port2")->valueint;
            device.device3 = cJSON_GetObjectItem(device_temp, "device_port3")->valueint;
            device.device4 = cJSON_GetObjectItem(device_temp, "device_port4")->valueint;
            flag1=true;
            // 使用进行设置
            printf("flag1:%d",flag1);
            free(pstr);
        }
        else
        {
            // // 解析消息中的JSON数据
            // // printf("root: %p\n", root);

            // pstr = cJSON_PrintUnformatted(root); // 无格式的CJson
            // // printf("%s\n\n", pstr);
            // // free(pstr);

            // // 获取params字段
            // params = cJSON_GetObjectItem(root, "params");

            // rgb_color = cJSON_GetObjectItem(params, "environment_variable");

            // pstr = cJSON_PrintUnformatted(rgb_color); // 无格式的CJson
            // printf("%s\n\n", pstr);

            // // 直接解析颜色参数，不保存在物模型属性的RGBColor字段中
            // rgb_led.led_switch = cJSON_GetObjectItem(rgb_color, "LEDSwicth")->valueint;
            // // pstr = cJSON_PrintUnformatted(rgb_led.led_switch); // 无格式的CJson
            // // rgb_led.led_switch = atoi(pstr);
            // // printf("%s\n\n", pstr);

            // rgb_led.red = cJSON_GetObjectItem(rgb_color, "Red")->valueint;
            // // pstr = cJSON_PrintUnformatted(rgb_led.red); // 无格式的CJson
            // // rgb_led.red = atoi(pstr);
            // // printf("%s\n\n", pstr);

            // rgb_led.green = cJSON_GetObjectItem(rgb_color, "Green")->valueint;
            // // pstr = cJSON_PrintUnformatted(rgb_led.green); // 无格式的CJson
            // // rgb_led.green = atoi(pstr);
            // // printf("%s\n\n", pstr);

            // rgb_led.blue = cJSON_GetObjectItem(rgb_color, "Blue")->valueint;
            // // pstr = cJSON_PrintUnformatted(rgb_led.blue); // 无格式的CJson
            // // rgb_led.blue = atoi(pstr);
            // // printf("%s\n\n", pstr);

            // // 此处增加接受消息后的处理。

            // // 更新网关状态

            // free(pstr);
            // // 更新RGB LED的状态
            // rgb_led_update();
            // rx_sta_bool = true;
            // // example_publish(); // 消息推送

            // printf("rgb_led: red=%d, blue=%d, green=%d\n", rgb_led.red, rgb_led.blue, rgb_led.green);
        }
        break;
    default:
        break;
    }
}

// 消息订阅函数
int example_subscribe(void *handle)
{
    int res = 0;
    const char *fmt = "/sys/%s/%s/thing/service/property/set"; // 订阅主题格式字符串
    const char *fmt_1 = "/%s/%s/user/getMsg";
    char *topic = NULL; // 存储订阅主题字符串的指针
    int topic_len = 0;  // 订阅主题字符串的长度

    topic_len = strlen(fmt) + strlen(DEMO_PRODUCT_KEY) + strlen(DEMO_DEVICE_NAME) + 1; // 计算订阅主题字符串的长度
    topic = HAL_Malloc(topic_len);                                                     // 分配内存用于存储订阅主题字符串
    if (topic == NULL)
    {
        EXAMPLE_TRACE("memory not enough"); // 内存分配失败的错误提示
        return -1;
    }

    memset(topic, 0, topic_len);                                                           // 清空订阅主题字符串的内存空间
    HAL_Snprintf(topic, topic_len, fmt, DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME);               // 格式化订阅主题字符串
    res = IOT_MQTT_Subscribe(handle, topic, IOTX_MQTT_QOS0, example_message_arrive, NULL); // 执行消息订阅操作
    if (res < 0)
    {
        EXAMPLE_TRACE("subscribe failed"); // 订阅失败的错误提示
        HAL_Free(topic);                   // 释放分配的内存空间
        return -1;
    }

    HAL_Free(topic); // 释放分配的内存空间

    // 属性上报响应事件订阅
    // sys/a1BC3i5kPNZ/${deviceName}/thing/event/property/post_reply
    topic_len = strlen(fmt_1) + strlen(DEMO_PRODUCT_KEY) + strlen(DEMO_DEVICE_NAME) + 1; // 计算订阅主题字符串的长度
    topic = HAL_Malloc(topic_len);                                                       // 分配内存用于存储订阅主题字符串
    if (topic == NULL)
    {
        EXAMPLE_TRACE("memory not enough"); // 内存分配失败的错误提示
        return -1;
    }

    memset(topic, 0, topic_len);                                                           // 清空订阅主题字符串的内存空间
    HAL_Snprintf(topic, topic_len, fmt_1, DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME);             // 格式化订阅主题字符串
    res = IOT_MQTT_Subscribe(handle, topic, IOTX_MQTT_QOS0, example_message_arrive, NULL); // 执行消息订阅操作
    if (res < 0)
    {
        EXAMPLE_TRACE("subscribe failed"); // 订阅失败的错误提示
        HAL_Free(topic);                   // 释放分配的内存空间
        return -1;
    }

    HAL_Free(topic); // 释放分配的内存空间
    return 0;
}

// 消息发布函数
int example_publish(void)
{
    char *payload = NULL; // 存储消息负载的指针
    int payload_len = 0;  // 消息负载的长度
    char *topic = NULL;   // 存储发布主题的指针
    int topic_len = 0;    // 发布主题的长度
    const char *payload_fmt = "{ \
        \"version\": \"1.0\", \
        \"params\": { \
            \"environment_variable\": { \
                \"wind_speed\": %d, \
                \"air_temperature\": %d, \
                \"air_humidity\": %d, \
                \"illuminance\": %d, \
                \"pressure\": %d, \
                \"noise\": %d, \
                \"soil_moisture\": %d, \
                \"soil_temperature\": %d, \
                \"soil_conductivity\": %d, \
                \"soil_PH\": %d \
            } \
        }, \
        \"method\": \"thing.event.property.post\" \
    }";                   // 消息负载格式字符串

    // 计算消息负载的长度
    payload_len = snprintf(NULL, 0, payload_fmt, environment_variable.wind_speed, environment_variable.air_temperature,
                           environment_variable.air_humidity, environment_variable.illuminance, environment_variable.pressure,
                           environment_variable.noise, environment_variable.soil_moisture, environment_variable.soil_temperature,
                           environment_variable.soil_conductivity, environment_variable.soil_PH) +
                  1;

    payload = HAL_Malloc(payload_len); // 分配内存用于存储消息负载
    if (payload == NULL)
    {
        EXAMPLE_TRACE("memory not enough"); // 内存分配失败的错误提示
        return -1;
    }

    snprintf(payload, payload_len, payload_fmt, environment_variable.wind_speed, environment_variable.air_temperature,
             environment_variable.air_humidity, environment_variable.illuminance, environment_variable.pressure,
             environment_variable.noise, environment_variable.soil_moisture, environment_variable.soil_temperature,
             environment_variable.soil_conductivity, environment_variable.soil_PH); // 格式化消息负载字符串

    // 计算发布主题的长度
    topic_len = strlen("/sys/%s/%s/thing/event/property/post") + strlen(DEMO_PRODUCT_KEY) + strlen(DEMO_DEVICE_NAME) + 1;
    topic = HAL_Malloc(topic_len); // 分配内存用于存储发布主题
    if (topic == NULL)
    {
        HAL_Free(payload);
        EXAMPLE_TRACE("memory not enough"); // 内存分配失败的错误提示
        return -1;
    }
    memset(topic, 0, topic_len);                                                                                // 清空发布主题的内存空间
    HAL_Snprintf(topic, topic_len, "/sys/%s/%s/thing/event/property/post", DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME); // 格式化发布主题字符串

    int res = IOT_MQTT_Publish_Simple(NULL, topic, IOTX_MQTT_QOS1, payload, payload_len); // 执行消息发布操作
    if (res < 0)
    {
        EXAMPLE_TRACE("publish failed"); // 发布失败的错误提示
    }
    else
    {
        EXAMPLE_TRACE("publish success"); // 发布成功的提示
    }

    HAL_Free(payload); // 释放分配的内存空间
    HAL_Free(topic);   // 释放分配的内存空间

    return res;
}

int gate_publish()
{
    char *payload = NULL; // 存储消息负载的指针
    int payload_len = 0;  // 消息负载的长度
    char *topic = NULL;   // 存储发布主题的指针
    int topic_len = 0;    // 发布主题的长度
    const char *payload_fmt = "{ \
        \"flag\": \"%d\", \
        \"face_detection\": \"%d\", \
        \"face_ID\": \"%d\", \
        \"scan_QR_code\": \"%d\", \
        \"QR_content\": \"%d\", \
        \"regulation\": \"%d\", \
        \"GPT_ID\": \"%d\" \
    }";                   // 消息负载格式字符串

    // 计算消息负载的长度
    payload_len = snprintf(NULL, 0, payload_fmt, control.flag, control.face_detection, control.face_ID, control.scan_QR_code, control.QR_content, control.regulation, control.GPT_ID) + 1;

    payload = HAL_Malloc(payload_len); // 分配内存用于存储消息负载
    if (payload == NULL)
    {
        EXAMPLE_TRACE("memory not enough"); // 内存分配失败的错误提示
        return -1;
    }

    snprintf(payload, payload_len, payload_fmt, control.flag, control.face_detection, control.face_ID, control.scan_QR_code, control.QR_content, control.regulation, control.GPT_ID); // 格式化消息负载字符串

    // 计算发布主题的长度
    topic_len = strlen("/%s/%s/user/pushMsg") + strlen(DEMO_PRODUCT_KEY) + strlen(DEMO_DEVICE_NAME) + 1;
    topic = HAL_Malloc(topic_len); // 分配内存用于存储发布主题
    if (topic == NULL)
    {
        HAL_Free(payload);
        EXAMPLE_TRACE("memory not enough"); // 内存分配失败的错误提示
        return -1;
    }
    memset(topic, 0, topic_len);                                                               // 清空发布主题的内存空间
    HAL_Snprintf(topic, topic_len, "/%s/%s/user/pushMsg", DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME); // 格式化发布主题字符串

    int res = IOT_MQTT_Publish_Simple(NULL, topic, IOTX_MQTT_QOS1, payload, payload_len); // 执行消息发布操作
    if (res < 0)
    {
        EXAMPLE_TRACE("publish failed"); // 发布失败的错误提示
    }
    else
    {
        EXAMPLE_TRACE("publish success"); // 发布成功的提示
    }

    HAL_Free(payload); // 释放分配的内存空间
    HAL_Free(topic);   // 释放分配的内存空间

    return 0;
}

void device_task(void *pvParams)
{
    char *payload = NULL; // 存储消息负载的指针
    int payload_len = 0;  // 消息负载的长度
    char *topic = NULL;   // 存储发布主题的指针
    int topic_len = 0;    // 发布主题的长度
    const char *payload_fmt = "{ \
        \"version\": \"1.0\", \
        \"params\": { \
            \"ex_device_ports\": { \
                \"device_port1\": %d, \
                \"device_port2\": %d, \
                \"device_port3\": %d, \
                \"device_port4\": %d \
            } \
        }, \
        \"method\": \"thing.event.property.post\" \
    }";                   // 消息负载格式字符串

    // 计算消息负载的长度
    payload_len = snprintf(NULL, 0, payload_fmt, device.device1, device.device2, device.device3, device.device4) + 1;

    payload = HAL_Malloc(payload_len); // 分配内存用于存储消息负载
    if (payload == NULL)
    {
        EXAMPLE_TRACE("memory not enough"); // 内存分配失败的错误提示
        vTaskDelete(NULL);
    }

    snprintf(payload, payload_len, payload_fmt, device.device1, device.device2, device.device3, device.device4); // 格式化消息负载字符串

    // 计算发布主题的长度
    topic_len = strlen("/sys/%s/%s/thing/service/property/set") + strlen(DEMO_PRODUCT_KEY) + strlen(DEMO_DEVICE_NAME) + 1;
    topic = HAL_Malloc(topic_len); // 分配内存用于存储发布主题
    if (topic == NULL)
    {
        HAL_Free(payload);
        EXAMPLE_TRACE("memory not enough"); // 内存分配失败的错误提示
        vTaskDelete(NULL);
    }
    memset(topic, 0, topic_len);                                                                                 // 清空发布主题的内存空间
    HAL_Snprintf(topic, topic_len, "/sys/%s/%s/thing/service/property/set", DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME); // 格式化发布主题字符串

    int res = IOT_MQTT_Publish_Simple(NULL, topic, IOTX_MQTT_QOS1, payload, payload_len); // 执行消息发布操作
    if (res < 0)
    {
        EXAMPLE_TRACE("publish failed"); // 发布失败的错误提示
    }
    else
    {
        EXAMPLE_TRACE("publish success"); // 发布成功的提示
    }

    HAL_Free(payload); // 释放分配的内存空间
    HAL_Free(topic);   // 释放分配的内存空间

    vTaskDelete(NULL);
}

// MQTT事件处理函数
void example_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    EXAMPLE_TRACE("msg->event_type : %d", msg->event_type); // 打印事件类型

    switch (msg->event_type)
    {
    case IOTX_MQTT_EVENT_UNDEF:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_UNDEF", msg->event_type); // 未定义事件类型
        break;
    case IOTX_MQTT_EVENT_DISCONNECT:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_DISCONNECT", msg->event_type); // MQTT断开连接事件
        break;
    case IOTX_MQTT_EVENT_RECONNECT:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_RECONNECT", msg->event_type); // MQTT重新连接事件
        break;
    case IOTX_MQTT_EVENT_SUBCRIBE_SUCCESS:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_SUBCRIBE_SUCCESS", msg->event_type); // MQTT订阅成功事件
        break;
    case IOTX_MQTT_EVENT_SUBCRIBE_TIMEOUT:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_SUBCRIBE_TIMEOUT", msg->event_type); // MQTT订阅超时事件
        break;
    case IOTX_MQTT_EVENT_SUBCRIBE_NACK:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_SUBCRIBE_NACK", msg->event_type); // MQTT订阅失败事件
        break;
    case IOTX_MQTT_EVENT_UNSUBCRIBE_SUCCESS:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_UNSUBCRIBE_SUCCESS", msg->event_type); // MQTT取消订阅成功事件
        break;
    case IOTX_MQTT_EVENT_UNSUBCRIBE_TIMEOUT:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_UNSUBCRIBE_TIMEOUT", msg->event_type); // MQTT取消订阅超时事件
        break;
    case IOTX_MQTT_EVENT_UNSUBCRIBE_NACK:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_UNSUBCRIBE_NACK", msg->event_type); // MQTT取消订阅失败事件
        break;
    case IOTX_MQTT_EVENT_PUBLISH_SUCCESS:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_PUBLISH_SUCCESS", msg->event_type); // MQTT消息发布成功事件
        break;
    case IOTX_MQTT_EVENT_PUBLISH_TIMEOUT:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_PUBLISH_TIMEOUT", msg->event_type); // MQTT消息发布超时事件
        break;
    case IOTX_MQTT_EVENT_PUBLISH_NACK:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_PUBLISH_NACK", msg->event_type); // MQTT消息发布失败事件
        break;
    case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_PUBLISH_RECEIVED", msg->event_type); // MQTT消息接收事件
        // 调用消息到达处理函数
        example_message_arrive(pcontext, pclient, msg);
        break;
    case IOTX_MQTT_EVENT_BUFFER_OVERFLOW:
        EXAMPLE_TRACE("msg->event_type : %d, IOTX_MQTT_EVENT_BUFFER_OVERFLOW", msg->event_type); // MQTT缓冲区溢出事件
        break;
    }
}

// MQTT主函数
void mqtt_main(void *pvParameters)
{
    void *pclient = NULL;          // MQTT客户端句柄
    int res = 0;                   // 返回结果变量
    iotx_mqtt_param_t mqtt_params; // MQTT连接参数

    // 获取产品认证参数
    HAL_GetProductKey(DEMO_PRODUCT_KEY);
    HAL_GetDeviceName(DEMO_DEVICE_NAME);
    HAL_GetDeviceSecret(DEMO_DEVICE_SECRET);

    EXAMPLE_TRACE("mqtt example"); // 打印示例信息

    memset(&mqtt_params, 0x0, sizeof(mqtt_params));

    mqtt_params.handle_event.h_fp = example_event_handle;

    // 根据参数构建MQTT客户端
    pclient = IOT_MQTT_Construct(&mqtt_params);
    if (NULL == pclient)
    {
        EXAMPLE_TRACE("MQTT construct failed"); // MQTT构建失败
        vTaskDelete(NULL);
    }

    // 订阅消息
    res = example_subscribe(pclient);
    if (res < 0)
    {
        // 连接失败，销毁客户端
        IOT_MQTT_Destroy(&pclient);
        vTaskDelete(NULL);
    }

    // 发布消息
    example_publish();
    gate_publish();

    while (1)
    {
        // 定时检测
        IOT_MQTT_Yield(pclient, 1000);
    }

    vTaskDelete(NULL);
}