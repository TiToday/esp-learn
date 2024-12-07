#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

// uart tx和rx, 可以修改
#define TXD_PIN (GPIO_NUM_0)
#define RXD_PIN (GPIO_NUM_1)

// 串口初始化，用于和传感器通讯
void uart_init(void)
{
    // uart配置参数
    const uart_config_t uart_config = {
        // 波特率
        .baud_rate = 9600, // 可能需要修改为115200,
        // 数据位
        .data_bits = UART_DATA_8_BITS,
        // 校验位
        .parity = UART_PARITY_DISABLE,
        // 停止位
        .stop_bits = UART_STOP_BITS_1,
        // 硬件流控模式
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        // 时钟源
        .source_clk = UART_SCLK_APB,
    };
    // 安装uart驱动, 配置UART RX 环形缓冲区大小, 不设置TX 环形缓冲区, 不设置UART 事件队列, 不设置中断
    uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0);
    // 配置uart参数
    uart_param_config(UART_NUM_1, &uart_config);
    // 配置uart引脚
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

// uart tx和rx, 可以修改
#define TXD2_PIN (GPIO_NUM_10)
#define RXD2_PIN (GPIO_NUM_11)

// static const char *TAG = "test";

// 串口2初始化，用于和中控屏通讯
void uart2_init(void)
{
    // uart配置参数
    const uart_config_t uart_config = {
        // 波特率
        .baud_rate = 9600, // 115200,
        // 数据位
        .data_bits = UART_DATA_8_BITS,
        // 校验位
        .parity = UART_PARITY_DISABLE,
        // 停止位
        .stop_bits = UART_STOP_BITS_1,
        // 硬件流控模式
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        // 时钟源
        .source_clk = UART_SCLK_APB,
    };
    // 安装uart驱动, 配置UART RX 环形缓冲区大小, 不设置TX 环形缓冲区, 不设置UART 事件队列, 不设置中断
    uart_driver_install(UART_NUM_2, 1024 * 2, 0, 0, NULL, 0);
    // 配置uart参数
    uart_param_config(UART_NUM_2, &uart_config);
    // 配置uart引脚
    uart_set_pin(UART_NUM_2, TXD2_PIN, RXD2_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}
