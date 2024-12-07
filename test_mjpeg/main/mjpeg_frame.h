#ifndef _MJPEG_FRAME_H_
#define _MJPEG_FRAME_H_
#include <stdint.h>
#include <string.h>

//jpeg图像数据
typedef struct 
{
    uint8_t *frame;
    size_t len;
}jpeg_frame_data_t;

typedef struct 
{
    size_t buffer_size;
}jpeg_frame_cfg_t;

void jpeg_frame_config(jpeg_frame_cfg_t *cfg);
void jpeg_frame_start(const char* filename);
void jpeg_frame_stop(void);
void jpeg_frame_get_one(jpeg_frame_data_t* data);



#endif