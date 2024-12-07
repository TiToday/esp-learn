
#ifndef _UART_1_H__
#define _UART_1_H__

// 串口初始化
void uart_init(void);
// uart 发送线程
void tx_task(void *arg);
// uart 接收线程
void rx_task(void *arg);
// 串口2初始化
void uart2_init(void);
// uart2 发送线程
void tx2_task(void *arg);
// uart2 接收线程
void rx2_task(void *arg);
#endif
