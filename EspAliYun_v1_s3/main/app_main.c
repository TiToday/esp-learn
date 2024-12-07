#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"

#include "infra_compat.h"
#include "rgb_led.h"
#include "button.h"
#include "mqtt_solo.h"
#include "conn_mgr.h"
#include "nvs_storage.h"
/////++++++
#include "uart_1.h"
#include "esp_event.h"
#include "esp_system.h"
// #include "esp_log.h"
#include "driver/uart.h"
// #include "string.h"
#include "driver/gpio.h"
// #include "websocket.h"
/////主程序
static const int RX_BUF_SIZE = 1024;
char text_data[50] = "18888888";
char text_id[1];

#define EXAMPLE_WIFI_SSID "keyshuo"
#define EXAMPLE_WIFI_PASS "wx614481987"
// #define EXAMPLE_WIFI_SSID "PROV_123456"
// #define EXAMPLE_WIFI_PASS "244466666"
// #define EXAMPLE_WIFI_SSID "208"
// #define EXAMPLE_WIFI_PASS "iot208208208"
// #define EXAMPLE_WIFI_SSID "508"
// #define EXAMPLE_WIFI_PASS "508508508"
// #define EXAMPLE_WIFI_SSID "小康师兄"
// #define EXAMPLE_WIFI_PASS "12345678"
extern environment_variable_t environment_variable;
// extern environment_variable;

static const char *TAG = "app main";

static bool mqtt_started = false;
// static int led_switch, red, green, blue;
unsigned char cmd1[] = {8, 05, 00, 00, 0xff, 00, 0x8C, 0xA3}; // D1 开
unsigned char cmd2[] = {8, 05, 00, 00, 0x00, 00, 0xCD, 0x53}; // D1 关

unsigned char cmd3[] = {8, 05, 00, 01, 0xff, 00, 0xDD, 0x63}; // D2 开
unsigned char cmd4[] = {8, 05, 00, 01, 0x00, 00, 0x9C, 0x93}; // D2 关

unsigned char cmd5[] = {8, 05, 00, 02, 0xff, 00, 0x2D, 0x63}; // D3 开
unsigned char cmd6[] = {8, 05, 00, 02, 0x00, 00, 0x6C, 0x93}; // D3 关

unsigned char cmd7[] = {8, 05, 00, 03, 0xff, 00, 0x7C, 0xA3}; // D4 开
unsigned char cmd8[] = {8, 05, 00, 03, 0x00, 00, 0x3D, 0x53}; // D4 关

bool flag1 = false;
/////+++++
// 两个输出引脚
#define GPIO_OUTPUT_IO_0 2
#define GPIO_OUTPUT_IO_1 3
// 引脚选择器
#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_OUTPUT_IO_0) | (1ULL << GPIO_OUTPUT_IO_1))

// 数据写的命令格式
char tx_qx1[] = {01, 03, 00, 00, 00, 01, 0x84, 0x0A}; // 风速传感器
char tx_qx2[] = {02, 03, 00, 02, 00, 02, 0x65, 0xF8}; // 空气温湿度传感器
char tx_qx3[] = {03, 03, 00, 07, 00, 02, 0x74, 0x28}; // 光照传感器
char tx_qx4[] = {04, 03, 00, 00, 00, 06, 0xC5, 0x9D}; // 大气压力传感器
char tx_qx5[] = {05, 03, 00, 00, 00, 01, 0x85, 0x8E}; // 噪声传感器
char tx_qx6[] = {06, 03, 00, 00, 00, 04, 0x45, 0xBE}; // 土壤传感器

char tx_qx7[] = {07, 01, 00, 00, 00, 01, 0x84, 0x0a};  // 泵1 开
char tx_qx8[] = {07, 01, 00, 00, 00, 00, 0x84, 0x0a};  // 泵1 关
char tx_qx9[] = {07, 02, 00, 00, 00, 01, 0x84, 0x0a};  // 泵2 开
char tx_qx10[] = {07, 02, 00, 00, 00, 00, 0x84, 0x0a}; // 泵2 关

char tx_sta = 0;          // 发送状态
char rx_sta = 0xff;       // 接受状态
char *cbuff;              // 字符缓冲区指针
char rx_qx[40];           // 存储字符接受数据
uint8_t rx_buf[100];      // 存储字节接收数据
int fs_buf;               // 文件大小
ulong gz_buf;             //
int dq_buf;               // 缓冲区大小
char cgq_buf[40];         // 存储传感器数据
bool tx_sta_bool = false; // 发送状态
bool rx_sta_bool = false; // 接受状态
int tx_counter = 0;       // 发布信息时长

extern rgb_led_t rgb_led; // 泵开关

// 传感器二进制数据转ASCLL码
char hex_ascii(char hex)
{
  if (hex <= 9)
    return 0x30 + hex;
  else
    return 0x37 + hex;
}

// uart 发送线程
void tx_task(void *arg)
{

  char cgq_buf[40];
  while (1)
  {
    // ESP_LOGI(TAG, "xxxxxx=%d\n",rx_buf[0]);
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    // ESP_LOGI(TAG, "xxxxxRead %d bytes: '%d'", 9, rx_buf[1]);
    // vTaskDelay(100 / portTICK_PERIOD_MS);

    ESP_LOG_BUFFER_HEXDUMP(TAG, rx_buf, 9, ESP_LOG_INFO);

    switch (rx_buf[0]) // 根据读缓冲区判断传感器
    {
    case 1: // 风速传感器 03H04H /10 = m/s
      // 解析风速
      gz_buf = rx_buf[3];
      gz_buf <<= 8;
      gz_buf += rx_buf[4];
      // 保存风速
      environment_variable.wind_speed = gz_buf;
      rx_buf[0] = 0xff;
      // 构建结构发送字符串
      memset(cgq_buf, 0, sizeof(cgq_buf));
      strcpy(cgq_buf, "t");
      strcat(cgq_buf, "1");
      // 淘晶池屏幕的t1控件
      strcat(cgq_buf, ".txt=\"");

      // 字符数组保存风速值
      text_data[0] = hex_ascii(gz_buf / 1000);         // 3
      text_data[1] = hex_ascii((gz_buf % 1000) / 100); // 3

      text_data[2] = hex_ascii((gz_buf % 100) / 10); // 2
      text_data[3] = '.';                            // 1
      text_data[4] = hex_ascii(gz_buf % 10);         // 1
      text_data[5] = 'm';
      text_data[6] = '/';
      text_data[7] = 's';
      text_data[8] = '\0';
      strcat(cgq_buf, text_data);
      strcat(cgq_buf, "\"");

      // 写入UART2串口通讯
      uart_write_bytes(UART_NUM_2, cgq_buf, strlen(cgq_buf));
      // 结束标识
      uart_write_bytes(UART_NUM_2, "\xff\xff\xff", strlen("\xff\xff\xff"));
      vTaskDelay(100 / portTICK_PERIOD_MS);
      break;
    case 2: // 空气温湿度传感器
      gz_buf = rx_buf[3];
      gz_buf <<= 8;
      gz_buf += rx_buf[4];

      environment_variable.air_temperature = gz_buf; // 空气温度

      // gz_buf <<= 8;
      // gz_buf += rx_buf[5];
      // gz_buf <<= 8;
      // gz_buf += rx_buf[6];

      // gz_buf =0xfffffff;
      rx_buf[0] = 0xff;
      // gz_buf *= 14;
      // gz_buf /= 100;
      //             ESP_LOGI(TAG, "qwex=%d\n",gz_buf);
      memset(cgq_buf, 0, sizeof(cgq_buf));
      strcpy(cgq_buf, "t");
      // strcat(cgq_buf, hex_ascii(rx_buf[0]));
      strcat(cgq_buf, "2");
      strcat(cgq_buf, ".txt=\"");

      text_data[0] = hex_ascii(gz_buf / 1000);         // 3
      text_data[1] = hex_ascii((gz_buf % 1000) / 100); // 3
      text_data[2] = '.';
      text_data[3] = hex_ascii((gz_buf % 100) / 10); // 2

      text_data[4] = hex_ascii(gz_buf % 10); // 1
      // text_data[5] = 'C';
      text_data[5] = 0xA1;
      text_data[6] = 0xE6;
      text_data[7] = '\0';
      strcat(cgq_buf, text_data);
      strcat(cgq_buf, "\"");

      uart_write_bytes(UART_NUM_2, cgq_buf, strlen(cgq_buf));
      uart_write_bytes(UART_NUM_2, "\xff\xff\xff", strlen("\xff\xff\xff"));
      vTaskDelay(100 / portTICK_PERIOD_MS);

      gz_buf = rx_buf[5];
      gz_buf <<= 8;
      gz_buf += rx_buf[6];
      environment_variable.air_humidity = gz_buf; // 空气湿度
      memset(cgq_buf, 0, sizeof(cgq_buf));
      strcpy(cgq_buf, "t");
      // strcat(cgq_buf, hex_ascii(rx_buf[0]));
      strcat(cgq_buf, "3");
      strcat(cgq_buf, ".txt=\"");

      text_data[0] = hex_ascii(gz_buf / 1000);         // 3
      text_data[1] = hex_ascii((gz_buf % 1000) / 100); // 3
      text_data[2] = '.';
      text_data[3] = hex_ascii((gz_buf % 100) / 10); // 2

      text_data[4] = hex_ascii(gz_buf % 10); // 1
      text_data[5] = '%';
      text_data[6] = 'R';
      text_data[7] = 'H';
      text_data[8] = '\0';
      strcat(cgq_buf, text_data);
      strcat(cgq_buf, "\"");
      // gpio_set_level(GPIO_OUTPUT_IO_1, 0);//指向LCD屏

      uart_write_bytes(UART_NUM_2, cgq_buf, strlen(cgq_buf));
      uart_write_bytes(UART_NUM_2, "\xff\xff\xff", strlen("\xff\xff\xff"));
      vTaskDelay(100 / portTICK_PERIOD_MS);

      break;

    case 3: // 光照传感器
      gz_buf = rx_buf[3];
      gz_buf <<= 8;
      gz_buf += rx_buf[4];
      gz_buf <<= 8;
      gz_buf += rx_buf[5];
      gz_buf <<= 8;
      gz_buf += rx_buf[6];
      environment_variable.illuminance = gz_buf; // 光照度
      // gz_buf =0xfffffff;
      rx_buf[0] = 0xff;
      gz_buf *= 14;
      gz_buf /= 100;
      //          ESP_LOGI(TAG, "qwex=%d\n",gz_buf);
      memset(cgq_buf, 0, sizeof(cgq_buf));
      strcpy(cgq_buf, "t");
      // strcat(cgq_buf, hex_ascii(rx_buf[0]));
      text_id[0] = '4';
      strcat(cgq_buf, "4");
      strcat(cgq_buf, ".txt=\"");

      text_data[0] = hex_ascii(gz_buf / 100000000);              // 9
      text_data[1] = hex_ascii((gz_buf % 100000000) / 10000000); // 8

      // text_data[2] = hex_ascii(gz_buf/10000000);//8
      text_data[2] = hex_ascii((gz_buf % 10000000) / 1000000); // 7

      // text_data[4] = hex_ascii(gz_buf/1000000);//7
      text_data[3] = hex_ascii((gz_buf % 1000000) / 100000); // 6

      // text_data[6] = hex_ascii(gz_buf/100000);//6
      text_data[4] = hex_ascii((gz_buf % 100000) / 10000); // 5
      text_data[5] = '.';
      // text_data[8] = hex_ascii(gz_buf/10000);//6
      text_data[6] = hex_ascii((gz_buf % 10000) / 1000); // 4

      text_data[7] = hex_ascii((gz_buf % 1000) / 100); // 3
      text_data[8] = hex_ascii((gz_buf % 100) / 10);   // 2
      text_data[9] = hex_ascii(gz_buf % 10);           // 1
      text_data[10] = 'L';
      text_data[11] = 'u';
      text_data[12] = 'x';
      text_data[13] = '\0';
      strcat(cgq_buf, text_data);
      strcat(cgq_buf, "\"");
      // gpio_set_level(GPIO_OUTPUT_IO_1, 0);//指向LCD屏

      uart_write_bytes(UART_NUM_2, cgq_buf, strlen(cgq_buf));
      uart_write_bytes(UART_NUM_2, "\xff\xff\xff", strlen("\xff\xff\xff"));
      vTaskDelay(100 / portTICK_PERIOD_MS);
      break;

    case 4: // 大气压力传感器
      gz_buf = rx_buf[3];
      gz_buf <<= 8;
      gz_buf += rx_buf[4];
      environment_variable.pressure = gz_buf; // 气压
      // gz_buf <<= 8;
      // gz_buf += rx_buf[5];
      // gz_buf <<= 8;
      // gz_buf += rx_buf[6];

      // gz_buf =0xfffffff;
      rx_buf[0] = 0xff;
      // gz_buf *= 14;
      // gz_buf /= 100;
      //          ESP_LOGI(TAG, "qwex=%d\n",gz_buf);
      memset(cgq_buf, 0, sizeof(cgq_buf));
      strcpy(cgq_buf, "t");
      // strcat(cgq_buf, hex_ascii(rx_buf[0]));
      strcat(cgq_buf, "5");
      strcat(cgq_buf, ".txt=\"");

      text_data[0] = hex_ascii(gz_buf / 1000);         // 3
      text_data[1] = hex_ascii((gz_buf % 1000) / 100); // 3

      text_data[2] = hex_ascii((gz_buf % 100) / 10); // 2
      text_data[3] = '.';
      text_data[4] = hex_ascii(gz_buf % 10); // 1
      text_data[5] = 'k';
      text_data[6] = 'p';
      text_data[7] = 'a';
      // text_data[5] = 'C';

      text_data[8] = '\0';
      strcat(cgq_buf, text_data);
      strcat(cgq_buf, "\"");
      // gpio_set_level(GPIO_OUTPUT_IO_1, 0);//指向LCD屏

      uart_write_bytes(UART_NUM_2, cgq_buf, strlen(cgq_buf));
      uart_write_bytes(UART_NUM_2, "\xff\xff\xff", strlen("\xff\xff\xff"));
      vTaskDelay(100 / portTICK_PERIOD_MS);

      break;
    case 5: // 噪声传感器
      gz_buf = rx_buf[3];
      gz_buf <<= 8;
      gz_buf += rx_buf[4];
      environment_variable.noise = gz_buf; // 噪声
      // gz_buf <<= 8;
      // gz_buf += rx_buf[5];
      // gz_buf <<= 8;
      // gz_buf += rx_buf[6];

      // gz_buf =0xfffffff;
      rx_buf[0] = 0xff;
      // gz_buf *= 14;
      // gz_buf /= 100;
      ///          ESP_LOGI(TAG, "qwex=%d\n",gz_buf);
      memset(cgq_buf, 0, sizeof(cgq_buf));
      strcpy(cgq_buf, "t");
      // strcat(cgq_buf, hex_ascii(rx_buf[0]));
      strcat(cgq_buf, "6");
      strcat(cgq_buf, ".txt=\"");

      text_data[0] = hex_ascii(gz_buf / 1000);         // 3
      text_data[1] = hex_ascii((gz_buf % 1000) / 100); // 3

      text_data[2] = hex_ascii((gz_buf % 100) / 10); // 2
      text_data[3] = '.';
      text_data[4] = hex_ascii(gz_buf % 10); // 1
      text_data[5] = 'd';
      text_data[6] = 'B';
      text_data[7] = '\0';
      strcat(cgq_buf, text_data);
      strcat(cgq_buf, "\"");
      // gpio_set_level(GPIO_OUTPUT_IO_1, 0);//指向LCD屏

      uart_write_bytes(UART_NUM_2, cgq_buf, strlen(cgq_buf));
      uart_write_bytes(UART_NUM_2, "\xff\xff\xff", strlen("\xff\xff\xff"));
      vTaskDelay(100 / portTICK_PERIOD_MS);

      break;
    case 6: // 土壤传感器
      // 温度
      gz_buf = rx_buf[5];
      gz_buf <<= 8;
      gz_buf += rx_buf[6];
      environment_variable.soil_temperature = gz_buf; // 土壤温度
      // gz_buf <<= 8;
      // gz_buf += rx_buf[5];
      // gz_buf <<= 8;
      // gz_buf += rx_buf[6];

      // gz_buf =0xfffffff;
      rx_buf[0] = 0xff;
      // gz_buf *= 14;
      // gz_buf /= 100;
      //        ESP_LOGI(TAG, "qwex=%d\n",gz_buf);
      memset(cgq_buf, 0, sizeof(cgq_buf));
      strcpy(cgq_buf, "t");
      // strcat(cgq_buf, hex_ascii(rx_buf[0]));
      strcat(cgq_buf, "8");
      strcat(cgq_buf, ".txt=\"");

      text_data[0] = hex_ascii(gz_buf / 1000);         // 3
      text_data[1] = hex_ascii((gz_buf % 1000) / 100); // 3

      text_data[2] = hex_ascii((gz_buf % 100) / 10); // 2
      text_data[3] = '.';
      text_data[4] = hex_ascii(gz_buf % 10); // 1
      text_data[5] = 'C';
      text_data[5] = 0xA1;
      text_data[6] = 0xE6;

      text_data[7] = '\0';
      strcat(cgq_buf, text_data);
      strcat(cgq_buf, "\"");
      ///////////////////////////////////////////////////////////////

      // gpio_set_level(GPIO_OUTPUT_IO_1, 0);//指向LCD屏

      uart_write_bytes(UART_NUM_2, cgq_buf, strlen(cgq_buf));
      uart_write_bytes(UART_NUM_2, "\xff\xff\xff", strlen("\xff\xff\xff"));
      vTaskDelay(100 / portTICK_PERIOD_MS);

      gz_buf = rx_buf[3];
      gz_buf <<= 8;
      gz_buf += rx_buf[4];
      environment_variable.soil_moisture = gz_buf; // 土壤水份

      //          ESP_LOGI(TAG, "qwex=%d\n",gz_buf);
      memset(cgq_buf, 0, sizeof(cgq_buf));
      strcpy(cgq_buf, "t");
      // strcat(cgq_buf, hex_ascii(rx_buf[0]));
      strcat(cgq_buf, "7");
      strcat(cgq_buf, ".txt=\"");

      text_data[0] = hex_ascii(gz_buf / 1000);         // 3
      text_data[1] = hex_ascii((gz_buf % 1000) / 100); // 3
      text_data[2] = '.';
      text_data[3] = hex_ascii((gz_buf % 100) / 10); // 2

      text_data[4] = hex_ascii(gz_buf % 10); // 1
      text_data[5] = '%';
      text_data[6] = 'R';
      text_data[7] = 'H';
      text_data[8] = '\0';
      strcat(cgq_buf, text_data);
      strcat(cgq_buf, "\"");
      // gpio_set_level(GPIO_OUTPUT_IO_1, 0);//指向LCD屏

      uart_write_bytes(UART_NUM_2, cgq_buf, strlen(cgq_buf));
      uart_write_bytes(UART_NUM_2, "\xff\xff\xff", strlen("\xff\xff\xff"));
      vTaskDelay(100 / portTICK_PERIOD_MS);

      gz_buf = rx_buf[7];
      gz_buf <<= 8;
      gz_buf += rx_buf[8];
      environment_variable.soil_conductivity = gz_buf; // 土壤电导率
      //           ESP_LOGI(TAG, "qwex=%d\n",gz_buf);
      memset(cgq_buf, 0, sizeof(cgq_buf));
      strcpy(cgq_buf, "t");
      // strcat(cgq_buf, hex_ascii(rx_buf[0]));
      strcat(cgq_buf, "9");
      strcat(cgq_buf, ".txt=\"");

      text_data[0] = hex_ascii(gz_buf / 1000);         // 3
      text_data[1] = hex_ascii((gz_buf % 1000) / 100); // 3
      text_data[2] = hex_ascii((gz_buf % 100) / 10);   // 2

      text_data[3] = hex_ascii(gz_buf % 10); // 1
      text_data[5] = 'u';
      text_data[6] = 's';
      text_data[7] = '/';
      text_data[8] = 'c';
      text_data[9] = 'm';
      text_data[10] = '\0';
      strcat(cgq_buf, text_data);
      strcat(cgq_buf, "\"");
      // gpio_set_level(GPIO_OUTPUT_IO_1, 0);//指向LCD屏

      uart_write_bytes(UART_NUM_2, cgq_buf, strlen(cgq_buf));
      uart_write_bytes(UART_NUM_2, "\xff\xff\xff", strlen("\xff\xff\xff"));
      vTaskDelay(100 / portTICK_PERIOD_MS);

      gz_buf = rx_buf[9];
      gz_buf <<= 8;
      gz_buf += rx_buf[10];
      environment_variable.soil_PH = gz_buf; // PH值
      //            ESP_LOGI(TAG, "qwex=%d\n",gz_buf);
      memset(cgq_buf, 0, sizeof(cgq_buf));
      strcpy(cgq_buf, "t");
      // strcat(cgq_buf, hex_ascii(rx_buf[0]));
      strcat(cgq_buf, "10");
      strcat(cgq_buf, ".txt=\"");

      text_data[0] = hex_ascii((gz_buf % 100) / 10); // 2
      text_data[1] = '.';
      text_data[2] = hex_ascii(gz_buf % 10); // 1

      text_data[3] = '\0';
      strcat(cgq_buf, text_data);
      strcat(cgq_buf, "\"");
      // gpio_set_level(GPIO_OUTPUT_IO_1, 0);//指向LCD屏

      uart_write_bytes(UART_NUM_2, cgq_buf, strlen(cgq_buf));
      uart_write_bytes(UART_NUM_2, "\xff\xff\xff", strlen("\xff\xff\xff"));
      vTaskDelay(100 / portTICK_PERIOD_MS);

      tx_sta_bool = true; // 设置发送状态
      break;
    }

    if (tx_sta_bool == true)
    {
      // 当网关推送20次
      if (tx_counter >= 20)
      {
        // 推送到阿里云
        example_publish();
        tx_counter = 0;
      }
      tx_counter++;
      tx_sta_bool = false; // 发送状态修改为false
      //
    }

    if (rx_sta_bool) // 读取状态
    {
      if (rgb_led.red)
      {
        cbuff = tx_qx7; // 泵1 开
      }
      else
        cbuff = tx_qx8; // 泵1 关
      uart_write_bytes(UART_NUM_1, cbuff, 8);
      // 任务调度，挂起该任务，延迟300 / portTICK_PERIOD_MS
      vTaskDelay(300 / portTICK_PERIOD_MS);
      if (rgb_led.green)
      {
        cbuff = tx_qx9; // 泵2 开
      }
      else
        cbuff = tx_qx10; // 泵2 关

      uart_write_bytes(UART_NUM_1, cbuff, 8);
      vTaskDelay(300 / portTICK_PERIOD_MS);
      rx_sta_bool = false;
    }

    // 根据设备，进行顺序写
    switch (tx_sta)
    {
    case 0:
      cbuff = tx_qx1;
      tx_sta++;
      break;
    case 1:
      cbuff = tx_qx2;
      tx_sta++;
      break;
    case 2:
      cbuff = tx_qx3;
      tx_sta++;
      break;
    case 3:
      cbuff = tx_qx4;
      tx_sta++;
      break;
    case 4:
      cbuff = tx_qx5;
      tx_sta++;
      break;
    case 5:
      cbuff = tx_qx6;
      tx_sta = 0;
      break;
    }

    gpio_set_level(GPIO_OUTPUT_IO_1, 1); // 串口指向RS485
    gpio_set_level(GPIO_OUTPUT_IO_0, 1); // 发送
    uart_write_bytes(UART_NUM_1, cbuff, 8);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_OUTPUT_IO_0, 0); // 接收
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "%d\n", cbuff[0]);

    // 测试用
    char buff[40];
    memset(buff, 0, sizeof(buff));
    strcpy(buff, "t");
    strcat(buff, text_id);
    strcat(buff, ".txt=\"");
    strcat(buff, text_data);
    strcat(buff, "\"");
    text_data[0]++;
    if (text_data[0] == 0x3a)
      text_data[0] = 0x30;
    text_id[0]++;
    if (text_id[0] == 0x37)
      text_id[0] = 0x34;

    // 延时2000ms
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// uart 接收线程
void rx_task(void *arg)
{

  char command[20];

  while (1)
  {
    // 进入函数日志
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "接收线程\n");
    // 动态分配内存data
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    while (1)
    {
      // 从UART_NUM_1接收数据到data, 最大接收RX_BUF_SIZE个数据, 超时时长1000ms
      const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
      if (rxBytes > 0)
      {
        // 字符串结束符0
        data[rxBytes] = 0;
        for (int i = 0; i <= rxBytes; i++)
        {
          rx_buf[i] = *(data + i);
        }

        // 字符串打印接收数据
        // ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
        // ESP_LOGI(RX_TASK_TAG, "Count %d", tx_counter);
        // 16进制打印接收数据
        // ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        // ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, rx_buf, rxBytes, ESP_LOG_INFO);
        if (flag1)
        {
          ESP_LOGI(TAG, "继电器");
          // 收到下发命令，开始检测命令并控制继电器
          if (device.device1)
          {
            uart_write_bytes(UART_NUM_1, cmd1, sizeof(cmd1)); // D1开
            ESP_LOGI(TAG, "D1开");
          }
          else
          {
            uart_write_bytes(UART_NUM_1, cmd2, sizeof(cmd2)); // D1关
            ESP_LOGI(TAG, "D1关");
          }
          vTaskDelay(100 / portTICK_PERIOD_MS); // 降低CPU占用

          if (device.device2)
          {
            uart_write_bytes(UART_NUM_1, cmd3, sizeof(cmd3)); // D2开
            ESP_LOGI(TAG, "D2开");
          }
          else
          {
            uart_write_bytes(UART_NUM_1, cmd4, sizeof(cmd4)); // D2关
            ESP_LOGI(TAG, "D2关");
          }
          vTaskDelay(100 / portTICK_PERIOD_MS); // 降低CPU占用

          if (device.device3)
          {
            uart_write_bytes(UART_NUM_1, cmd5, sizeof(cmd5)); // D3开
            ESP_LOGI(TAG, "D3开");
          }
          else
          {
            uart_write_bytes(UART_NUM_1, cmd6, sizeof(cmd6)); // D3关
            ESP_LOGI(TAG, "D3关");
          }
          vTaskDelay(100 / portTICK_PERIOD_MS); // 降低CPU占用

          if (device.device4)
          {
            uart_write_bytes(UART_NUM_1, cmd7, sizeof(cmd7)); // D4开
            ESP_LOGI(TAG, "D4开");
          }
          else
          {
            uart_write_bytes(UART_NUM_1, cmd8, sizeof(cmd8)); // D4关
            ESP_LOGI(TAG, "D4关");
          }
          vTaskDelay(100 / portTICK_PERIOD_MS); // 降低CPU占用

          flag1 = false;
        }
      }
    }
    // 动态释放内存data
    // printf(flag1)
    free(data);
    // 处理命令

    vTaskDelay(100 / portTICK_PERIOD_MS); // 降低CPU占用
  }
}

// uart2中控屏接收线程
void rx2_task(void *arg)
{
  // // 设置三个临时变量存储，用于存储转换数值
  // char temp1[32];
  // char temp2[32];
  // char temp3[32];

  int buf_index = 0;
  // // 临时变量使用索引
  // int temp_index=0;
  static const char *RX_TASK_TAG = "UART2_RX_TASK";
  esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);

  ESP_LOGI(TAG, "接收线程\n");
  uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
  while (1)
  {
    char temp1[32];
    char temp2[32];
    char temp3[32];

    // 临时变量使用索引
    int temp_index = 0;
    const int rxBytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE, 100 / portTICK_RATE_MS);
    if (rxBytes > 0)
    {
      for (int i = 0; i < rxBytes; i++)
      {
        if (data[i] == 0x55 && data[i + 1] == 0x01)
        {
          // 找到包头，重置缓冲区索引
          buf_index = 0;
          i++;
        }
        else if (data[i] == 0xFF && data[i + 1] == 0xFF && data[i + 2] == 0xFF)
        {
          // 找到包尾，解析数据并存储
          rx_buf[buf_index] = '\0';

          if (temp_index == 0)
          {
            ESP_LOGI(RX_TASK_TAG, "Temp_index = %d\n", temp_index);
            strncpy(temp1, (const char *)rx_buf, sizeof(temp1) - 1);
            temp1[sizeof(temp1) - 1] = '\0';
            temp_index++;
            ESP_LOGI(RX_TASK_TAG, "Temp1 = %s\n", temp1);
          }
          else if (temp_index == 1)
          {
            ESP_LOGI(RX_TASK_TAG, "Temp_index = %d\n", temp_index);
            strncpy(temp2, (const char *)rx_buf, sizeof(temp2) - 1);
            temp2[sizeof(temp2) - 1] = '\0';
            temp_index++;
            ESP_LOGI(RX_TASK_TAG, "Temp1 = %s\n", temp2);
          }
          else if (temp_index == 2)
          {
            ESP_LOGI(RX_TASK_TAG, "Temp_index = %d\n", temp_index);
            strncpy(temp3, (const char *)rx_buf, sizeof(temp3) - 1);
            temp3[sizeof(temp3) - 1] = '\0';
            temp_index++;
            ESP_LOGI(RX_TASK_TAG, "Temp1 = %s\n", temp3);
          }
          else
          {
            ESP_LOGI(RX_TASK_TAG, "Insufficient number of temporary variables!!!");
          }

          ESP_LOGI(RX_TASK_TAG, "Received Data: %s", rx_buf);

          buf_index = 0; // 重置缓冲区索引
          i += 2;        // 跳过包尾的下两个字节
        }
        else
        {
          // 存储数据到缓冲区
          rx_buf[buf_index++] = data[i];
          if (buf_index >= RX_BUF_SIZE)
          {
            // 缓冲区溢出，重置缓冲区索引
            buf_index = 0;
            ESP_LOGW(RX_TASK_TAG, "Buffer Overflow!");
          }
        }
      }
      // 路径选择
      // 登陆
      if (strcmp(temp1, "login") == 0)
      {
        if (strcmp(temp2, "face") == 0)
        {
          control.face_detection = 1;
          control.flag = 1;
          gate_publish();
        }
        else
        {
          stpcpy(user.account, temp2);
          stpcpy(user.password, temp3);
          xTaskCreate(socket_task, "socket_task", 8192, NULL, 5, NULL);
        }
      }
      else if (strcmp(temp1, "device") == 0)
      {
        // 读取中控屏的开关信息
        // 发布对应属性设置
        // 接收改变，进行开关
        int temp = atoi(temp2);
        switch (temp)
        {
        case 1:
          if (strcmp(temp3, "on") == 0)
          {
            device.device1 = 1;
            xTaskCreate(device_task, "device_task", 8192, &temp, 5, NULL);
          }
          else
          {
            device.device1 = 0;
            xTaskCreate(device_task, "device_task", 8192, &temp, 5, NULL);
          }
          break;
        case 2:
          if (strcmp(temp3, "on") == 0)
          {
            device.device2 = 1;
            xTaskCreate(device_task, "device_task", 8192, &temp, 5, NULL);
          }
          else
          {
            device.device2 = 0;
            xTaskCreate(device_task, "device_task", 8192, &temp, 5, NULL);
          }
          break;
        case 3:
          if (strcmp(temp3, "on") == 0)
          {
            device.device3 = 1;
            xTaskCreate(device_task, "device_task", 8192, &temp, 5, NULL);
          }
          else
          {
            device.device3 = 0;
            xTaskCreate(device_task, "device_task", 8192, &temp, 5, NULL);
          }
          break;
        case 4:
          if (strcmp(temp3, "on") == 0)
          {
            device.device4 = 1;
            xTaskCreate(device_task, "device_task", 8192, &temp, 5, NULL);
          }
          else
          {
            device.device4 = 0;
            xTaskCreate(device_task, "device_task", 8192, &temp, 5, NULL);
          }
          break;
        default:
          break;
        }
      }
      // else if (strcmp(temp1, "monitor") == 0)
      // {
      // }
      // else
      // {
      // }
    }
  }
  // 动态释放内存data
  free(data);
}

static esp_err_t wifi_event_handle(void *ctx, system_event_t *event)
{
  switch (event->event_id)
  {
  case SYSTEM_EVENT_STA_GOT_IP:
    // 当Wi-Fi连接成功获取到IP地址时
    if (mqtt_started == false)
    {
      // 如果MQTT客户端尚未启动
      xTaskCreate(mqtt_main, "mqtt_example", 10240, NULL, 5, NULL);
      mqtt_started = true;
    }
    break;
  default:
    break;
  }
  return ESP_OK;
}

void app_main()
{
  // 初始化配置结构体
  gpio_config_t io_conf = {};
  // 禁用中断
  io_conf.intr_type = GPIO_INTR_DISABLE;
  // 设置为输出模式，将GPIO引脚配置为输出信号
  io_conf.mode = GPIO_MODE_OUTPUT;
  // 设置的引脚的位掩码，指定两个需要配置为输出模式的GPIO引脚
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  // 确保GPIO引脚在输出模式下不受其他电气状态的影响。禁用下拉和上拉电阻可以确保输出电平的准确性，避免因外部电路的干扰而产生误判。
  // 禁用下拉模式
  io_conf.pull_down_en = 0;
  // 禁用上拉模式
  io_conf.pull_up_en = 0;
  // 使用给定的设置配置GPIO
  gpio_config(&io_conf);
  // 设置引脚的电平状态
  gpio_set_level(GPIO_OUTPUT_IO_0, 1);
  gpio_set_level(GPIO_OUTPUT_IO_1, 0);
  // 延时2000ms
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  gpio_set_level(GPIO_OUTPUT_IO_0, 0);
  gpio_set_level(GPIO_OUTPUT_IO_1, 0);
  // 串口初始化
  uart_init();
  uart2_init();
  text_id[0] = 0x34;
  // 创建并启动串口接收线程rx_task
  xTaskCreate(rx_task, "uart_rx_task", 2048 * 2, NULL, configMAX_PRIORITIES, NULL);
  // 创建并启动串口接收线程rx_task
  xTaskCreate(rx2_task, "uart2_rx_task", 2048 * 2, NULL, configMAX_PRIORITIES, NULL);
  // 创建并启动串口发送线程tx_task
  xTaskCreate(tx_task, "uart_tx_task", 2048 * 2, NULL, configMAX_PRIORITIES - 1, NULL);

  nvs_init(); // 初始化 NVS
  button_init();
  rgb_led_init();
  conn_mgr_init();
  // 注册Wi-Fi事件处理函数wifi_event_handle
  conn_mgr_register_wifi_event(wifi_event_handle);
  // 设置Wi-Fi配置
  conn_mgr_set_wifi_config_ext((const uint8_t *)EXAMPLE_WIFI_SSID, strlen(EXAMPLE_WIFI_SSID), (const uint8_t *)EXAMPLE_WIFI_PASS, strlen(EXAMPLE_WIFI_PASS));

  IOT_SetLogLevel(IOT_LOG_INFO);

  // 启动连接管理器
  conn_mgr_start();

  // xTaskCreate(socket_task, "socket_task", 8192, NULL, 5, &network_connected_task);
  // xTaskCreate((void (*)(void *))gate_publish,"gate_publish",4096,NULL,4,&network_connected_task);
  // const char *websocket_uri = "ws://124.222.224.186:8800";
  // xTaskCreate(websocket_app_start, "websocket_app_start", 8192, NULL, 4, &network_connected_task);
}