#include "lvgl.h"
#include "esp_log.h"
#include "string.h"
#include "sdcard_file.h"
#include "ui_mjpeg_player.h"
#include "esp_spiffs.h"
#include "mjpeg_frame.h"
#include "img_converters.h"



//文件标题控件
static lv_obj_t *s_lv_file_title = NULL;

//文件列表
static lv_obj_t *s_lv_file_list = NULL;

//文件页面
static lv_obj_t *s_lv_file_page = NULL;

//播放页面
static lv_obj_t *s_lv_player_page = NULL;
//播放控件
static lv_obj_t *s_lv_player_img = NULL;

//返回按键图片控件
static lv_obj_t *s_lv_back_img = NULL;

//暂停按键图片控件
static lv_obj_t *s_lv_pause_img = NULL;

//播放定时器
static lv_timer_t *s_player_timer = NULL;

//停止播放标志
static bool s_pause_flag = false;


void lv_timer_player_cb(struct _lv_timer_t * t)
{
    static jpeg_frame_data_t frame_data = {0};
    if(frame_data.frame)
    {
        free(frame_data.frame);
        frame_data.frame = NULL;
        frame_data.len = 0;
    }

    if(s_pause_flag)
    {
        return;
    }

    jpeg_frame_get_one(&frame_data);

    if(frame_data.len)
    {
        static lv_img_dsc_t img_dsc;
        memset(&img_dsc, 0, sizeof(img_dsc));

        static uint8_t *rgb565_data = NULL;
        uint16_t width = 0;
        uint16_t height = 0;

        if(rgb565_data)
        {
            free(rgb565_data);
            rgb565_data = NULL;
        }

        if(jpg2rgb565(frame_data.frame, frame_data.len, &rgb565_data, &width, &height,JPG_SCALE_NONE))
        {
            img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
            img_dsc.header.w = width;
            img_dsc.header.h = height;
            img_dsc.data = rgb565_data;
            img_dsc.data_size = (uint32_t)width * (uint32_t)height * 2;
            lv_img_set_src(s_lv_player_img, &img_dsc);
        }

    }else
    {   
        lv_timer_del(s_player_timer);
        s_player_timer = NULL;

    }

}


void lv_list_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    switch (code)
    {
    case LV_EVENT_CLICKED:
        
        if(s_player_timer)
        {
            return;
        }
        const char *text = lv_list_get_btn_text(s_lv_file_list, obj);
        ESP_LOGE("text","%s",text);
        if(text)
        {
            //ESP_LOGE("text","strlen = %d,sizeof = %d",strlen(text),sizeof(text) / sizeof(text[0]));
            if(strstr(text, ".mjpeg") ||  strstr(text, ".MJPEG") || strstr(text, ".jpg") || strstr(text, ".MJP"))
            {
                ESP_LOGE("1","1");
                ESP_LOGE("text","%s",text);
                jpeg_frame_start(text);
                s_pause_flag = false;
                s_player_timer = lv_timer_create(lv_timer_player_cb,10,NULL);
                lv_scr_load(s_lv_player_page);
            }
        }
        break;
    
    default:
        break;
    }
}

void lv_player_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED)
    {
        if(obj == s_lv_back_img)
        {
            if(s_player_timer)
            {
                lv_timer_del(s_player_timer);
                s_player_timer = NULL;
            }
            jpeg_frame_stop();
            lv_scr_load(s_lv_file_page);
        }else if(obj == s_lv_pause_img)
        {
            s_pause_flag =!s_pause_flag;
        }
    }
   
}



void ui_mjpeg_create(void)
{
    lv_coord_t hor_res = lv_disp_get_hor_res(NULL);  //获取水平分辨率
    lv_coord_t ver_res = lv_disp_get_ver_res(NULL);  //获取垂直分辨率

    //创建新的文件页面
    s_lv_file_page = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_lv_file_page, lv_color_black(), 0);

    //创建文件标题
    s_lv_file_title = lv_label_create(s_lv_file_page);
    lv_label_set_text(s_lv_file_title, "File");
    lv_obj_set_size(s_lv_file_title, hor_res - 60, 30);
    lv_obj_align(s_lv_file_title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(s_lv_file_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_lv_file_title, lv_color_black(), 0);
    lv_obj_set_style_bg_color(s_lv_file_title, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_lv_file_title, LV_OPA_COVER, 0);

    //创建文件列表
    s_lv_file_list = lv_list_create(s_lv_file_page);
    lv_obj_set_size(s_lv_file_list, hor_res - 60, ver_res - 90);
    lv_obj_align(s_lv_file_list, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_text_font(s_lv_file_list, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_lv_file_list, lv_color_black(), 0);
    lv_obj_set_style_bg_color(s_lv_file_list, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_lv_file_list, LV_OPA_COVER, 0);

    //初始化文件系统
    spiffs_init("storage","/spiffs");
    const char (*filelist)[256] = NULL;
    int file_name_cnt = spiffs_filelist(&filelist);
    for (int i = 0; i < file_name_cnt; i++)
    {
        
        lv_obj_t *btn = lv_list_add_btn(s_lv_file_list, NULL, *filelist);
        lv_obj_add_event_cb(btn,lv_list_event_cb,LV_EVENT_CLICKED,NULL);
        filelist++;
    }




    //创建播放页面
    s_lv_player_page = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_lv_player_page, lv_color_black(), 0);

    //创建播放图片控件
    s_lv_player_img = lv_img_create(s_lv_player_page);
    lv_obj_align(s_lv_player_img, LV_ALIGN_TOP_MID, 0, 35);

    //创建返回按键图片控件
    s_lv_back_img = lv_imgbtn_create(s_lv_player_page);
    lv_obj_set_size(s_lv_back_img, 48, 48);
    lv_obj_align(s_lv_back_img, LV_ALIGN_BOTTOM_LEFT, 30, -20);
    lv_imgbtn_set_src(s_lv_back_img, LV_IMGBTN_STATE_RELEASED, NULL, "/img/back_img_48.png", NULL);
    lv_obj_add_event_cb(s_lv_back_img,lv_player_btn_event_cb,LV_EVENT_CLICKED,NULL);


    //创建暂停按键图片控件
    s_lv_pause_img = lv_imgbtn_create(s_lv_player_page);
    lv_obj_set_size(s_lv_pause_img, 48, 48);
    lv_obj_align(s_lv_pause_img, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_imgbtn_set_src(s_lv_pause_img, LV_IMGBTN_STATE_RELEASED, NULL, "/img/pause_img_48.png", NULL);
    lv_obj_add_event_cb(s_lv_pause_img,lv_player_btn_event_cb,LV_EVENT_CLICKED,NULL);

    jpeg_frame_cfg_t cfg = {
        .buffer_size = 100 * 1024,
    };
    jpeg_frame_config(&cfg);


    //显示文件页面
    lv_scr_load(s_lv_file_page);
}