#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define UART_NUM UART_NUM_1
#define UART_TX_PIN GPIO_NUM_10
#define UART_RX_PIN GPIO_NUM_11

static char uart_buffer[1024];
//static QueueHandle_t uart_queue;

void app_main(void)
{
    //uart_event_t uart_ev;
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, -1, -1));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, 1024, 1024, 0, NULL, 0));

    float p=3.124;
    //char *s=p;
    snprintf(uart_buffer, sizeof(uart_buffer), "t8.txt=\"%.3f\"\xff\xff\xff",p);

    while (1)
    {
        // if (xQueueReceive(uart_queue, &uart_ev, portMAX_DELAY))
        // {
        //     switch (uart_ev.type)
        //     {
        //     case UART_DATA:
        //         ESP_LOGI("UART", "Data received: %d bytes", uart_ev.size);
        //         uart_read_bytes(UART_NUM, uart_buffer, uart_ev.size, pdMS_TO_TICKS(1000));
        //         uart_write_bytes(UART_NUM, uart_buffer, uart_ev.size);
        //         break;
        //     case UART_FIFO_OVF:
        //         ESP_LOGI("UART", "FIFO overflow");
        //         uart_flush_input(UART_NUM);
        //         xQueueReset(uart_queue);
        //         break;
        //     case UART_BUFFER_FULL:
        //         ESP_LOGI("UART", "Buffer full");
        //         uart_flush_input(UART_NUM);
        //         xQueueReset(uart_queue);
        //         break;
        //     default:
        //         break;
        //     }
        //}

        // int rec = uart_read_bytes(UART_NUM, uart_buffer, 1024, pdMS_TO_TICKS(50));
        // if (rec)
        // {
        //     uart_write_bytes(UART_NUM, uart_buffer, rec);
        // }
        //p++;
        snprintf(uart_buffer, sizeof(uart_buffer), "t8.txt=\"%.3f\"\xff\xff\xff",p);
                uart_write_bytes(UART_NUM, uart_buffer, strlen(uart_buffer));
        vTaskDelay(pdMS_TO_TICKS(1000));
        
    }
    
}
