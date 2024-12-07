#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "mjpeg_frame.h"

#define TAG          "mjpeg_frame"
#define JPEG_SOI     0xFFD8
#define JPEG_EOI     0xFFD9
#define START_GET_EV   BIT0
#define STOP_GET_EV    BIT1

static jpeg_frame_cfg_t s_mjpeg_cfg = {0};
static int mjpeg_inited = 0;   //初始化标记
static int mjpeg_started = 0;  //启动标记

static EventGroupHandle_t mjpeg_event;
static QueueHandle_t mjpeg_queue;


void jpeg_frame_config(jpeg_frame_cfg_t *cfg)
{

    if(cfg->buffer_size < 4096)
    {
        return;
    }
    memcpy(&s_mjpeg_cfg, cfg, sizeof(jpeg_frame_cfg_t));
    mjpeg_inited = 1;

    if(!mjpeg_event)
    {
        mjpeg_event = xEventGroupCreate();
    }
    if(!mjpeg_queue)
    {
        mjpeg_queue = xQueueCreate(8, sizeof(jpeg_frame_data_t));
    }

}


static void jpeg_frame_task(void *param)
{

    char* filename = (const char*)param;

    ESP_LOGE(TAG, "start jpeg_frame_task %s", filename);
    FILE *f = fopen(filename, "rb");
    if(!f)
    {
        ESP_LOGE(TAG, "open file %s failed", filename);
        goto mjpeg_task_return;
    }

    uint8_t *read_buffer = (uint8_t*)malloc(s_mjpeg_cfg.buffer_size);
    if(!read_buffer)
    {
        ESP_LOGE(TAG, "malloc read buffer failed");
        goto mjpeg_task_return;
    }
    int jpeg_start = 0;      //是否找到jpg头
    size_t read_bytes = 0;   //每次实际读取到的字节数
    uint8_t *frame_buffer = NULL;  //截取到的jpg帧数据
    size_t frame_write_index = 0;  //当前截取到的jpg数据长度
    size_t frame_buffer_total_len = s_mjpeg_cfg.buffer_size;    //分配给jpg帧数据内存大小

    while((read_bytes = fread(read_buffer, 1, s_mjpeg_cfg.buffer_size, f)) > 0)
    {
        int soi_index = 0;   //jpg头在当前读取到的数据中的位置
        for(int i = 0; i < read_bytes - 1; i++)
        {
            uint16_t oi_flags = (read_buffer[i] << 8) | read_buffer[i + 1];
            if(!jpeg_start && oi_flags == JPEG_SOI)
            {
                soi_index = i;
                jpeg_start = 1;
            }else if(jpeg_start && oi_flags == JPEG_EOI)
            {
                int write_len = (i - soi_index + 1) + 1;
                if(frame_buffer)
                {
                    if(frame_write_index + write_len <= frame_buffer_total_len)
                    {
                        memcpy(&frame_buffer[frame_write_index], &read_buffer[soi_index], write_len);
                    }else
                    {
                        frame_buffer = (uint8_t*)realloc(frame_buffer, frame_write_index + write_len);
                        frame_buffer_total_len = frame_write_index + write_len;
                        memcpy(&frame_buffer[frame_write_index], &read_buffer[soi_index], write_len);
                    }
                }else
                {
                    if(frame_buffer_total_len < write_len)
                    {
                        frame_buffer_total_len = write_len;
                    }
                    frame_buffer = (uint8_t*)malloc(frame_buffer_total_len);
                    memcpy(&frame_buffer[frame_write_index], &read_buffer[soi_index], write_len);
                }
                frame_write_index += write_len;

                jpeg_frame_data_t frame_data = {
                    .frame = frame_buffer,
                    .len = frame_write_index,
                };

                EventBits_t ev = xEventGroupWaitBits(mjpeg_event, START_GET_EV | STOP_GET_EV, pdTRUE, pdFALSE, portMAX_DELAY);
                if(ev & START_GET_EV)
                {
                    xQueueSend(mjpeg_queue, &frame_data, portMAX_DELAY);
                }
                if(ev & STOP_GET_EV)
                {
                    if(frame_buffer)
                    {
                        free(frame_buffer);
                    }
                    goto mjpeg_task_return;
                }

                jpeg_start = 0;
                frame_write_index = 0;
                frame_buffer = NULL;


            }
        }

        if(jpeg_start)
        {
            size_t  write_len = s_mjpeg_cfg.buffer_size - soi_index;
            if(frame_buffer)
            {
                if(frame_write_index + write_len <= frame_buffer_total_len)
                {
                    memcpy(&frame_buffer[frame_write_index], &read_buffer[soi_index], write_len);
                }else
                {
                    frame_buffer = (uint8_t*)realloc(frame_buffer, frame_write_index + write_len);
                    frame_buffer_total_len = frame_write_index + write_len;
                    memcpy(&frame_buffer[frame_write_index], &read_buffer[soi_index], write_len);
                }
            }else
            {
                if(frame_buffer_total_len < write_len)
                {
                    frame_buffer_total_len = write_len;
                }
                frame_buffer = (uint8_t*)malloc(frame_buffer_total_len);
                memcpy(&frame_buffer[frame_write_index], &read_buffer[soi_index], write_len);
            }
            frame_write_index += write_len;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }


    mjpeg_task_return:
    if(f)
    {
        fclose(f);
    }
    mjpeg_started = 0;
    vTaskDelete(NULL);
}



void jpeg_frame_start(const char* filename)
{
    ESP_LOGE("2","2");
    ESP_LOGE("text","%s",filename);

    if(!mjpeg_inited)
    {
        ESP_LOGE("3","3");
        ESP_LOGE(TAG, "mjpeg_frame_config not called");
        return;
    }
    if(mjpeg_started)
    {
        ESP_LOGE("4","4");
        return;
    }
    
    static char jpeg_filename[8192];
    
    snprintf(jpeg_filename, sizeof(jpeg_filename) - 1, "/spiffs/%s", filename);
    //jpeg_filename[sizeof(jpeg_filename) - 1] = '\0'; // 确保字符串以 null 结尾
    ESP_LOGE("5","5");
    mjpeg_started = 1;
    ESP_LOGE("1","%s---------%s",jpeg_filename,filename);
    xTaskCreatePinnedToCore(jpeg_frame_task, "mjpeg_task", 8192, jpeg_filename, 6, NULL, 0);

}





void jpeg_frame_stop(void)
{

    xEventGroupSetBits(mjpeg_event, STOP_GET_EV);

}





void jpeg_frame_get_one(jpeg_frame_data_t* data)
{
    
    jpeg_frame_data_t frame_data = {0};
    if(!mjpeg_started)
    {
        return;
    }
    xEventGroupSetBits(mjpeg_event, START_GET_EV);
    if(xQueueReceive(mjpeg_queue, &frame_data, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
        memcpy(data, &frame_data, sizeof(jpeg_frame_data_t));
        return;
    }else
    {
        data->len = 0;
    }

}

