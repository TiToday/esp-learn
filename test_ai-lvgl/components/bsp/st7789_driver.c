/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <sys/cdefs.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "st7789v";

static esp_err_t panel_st7789v_del(esp_lcd_panel_t *panel);
static esp_err_t panel_st7789v_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_st7789v_init(esp_lcd_panel_t *panel);
static esp_err_t panel_st7789v_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_st7789v_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_st7789v_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_st7789v_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_st7789v_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_st7789v_disp_on_off(esp_lcd_panel_t *panel, bool off);

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    bool reset_level;
    int x_gap;
    int y_gap;
    uint8_t fb_bits_per_pixel;
    uint8_t madctl_val; // save current value of LCD_CMD_MADCTL register
    uint8_t colmod_cal; // save surrent value of LCD_CMD_COLMOD register
} st7789v_panel_t;

/**
 * 创建并初始化一个新的ST7789V液晶面板对象。
 *
 * @param io 液晶面板的IO句柄，用于与硬件通信。
 * @param panel_dev_config 液晶面板的配置参数，包括重置引脚和颜色设置等。
 * @param ret_panel 输出参数，返回创建的液晶面板对象句柄。
 * @return esp_err_t 返回操作结果，ESP_OK表示成功，其他值表示错误。
 */
esp_err_t esp_lcd_new_panel_st7789v(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_err_t ret = ESP_OK; // 初始化返回值为ESP_OK，表示操作成功。
    st7789v_panel_t *st7789v = NULL; // 定义并初始化ST7789V面板对象指针。
    // 检查输入参数是否有效，如果无效则返回错误。
    ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    // 分配内存用于存储ST7789V面板对象。
    st7789v = calloc(1, sizeof(st7789v_panel_t));
    // 检查内存分配是否成功，如果失败则返回错误。
    ESP_GOTO_ON_FALSE(st7789v, ESP_ERR_NO_MEM, err, TAG, "no mem for st7789v panel");

    // 如果配置了重置引脚，则配置对应的GPIO。
    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        // 检查GPIO配置是否成功，如果失败则返回错误。
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    // 根据配置的颜色空间设置ST7789V的显示模式。
    switch (panel_dev_config->color_space) {
    case ESP_LCD_COLOR_SPACE_RGB:
        st7789v->madctl_val = 0;
        break;
    case ESP_LCD_COLOR_SPACE_BGR:
        st7789v->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        // 如果颜色空间不支持，则返回错误。
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
        break;
    }

    // 根据配置的每像素位数设置颜色模式和帧缓冲区的位深度。
    uint8_t fb_bits_per_pixel = 0;
    switch (panel_dev_config->bits_per_pixel) {
    case 16: // RGB565
        st7789v->colmod_cal = 0x55;
        fb_bits_per_pixel = 16;
        break;
    case 18: // RGB666
        st7789v->colmod_cal = 0x66;
        // 每个颜色分量（R/G/B）占用一个字节的高6位，因此一个像素需要3个完整的字节。
        fb_bits_per_pixel = 24;
        break;
    default:
        // 如果每像素位数不支持，则返回错误。
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    // 设置ST7789V面板对象的成员变量。
    st7789v->io = io;
    st7789v->fb_bits_per_pixel = fb_bits_per_pixel;
    st7789v->reset_gpio_num = panel_dev_config->reset_gpio_num;
    st7789v->reset_level = panel_dev_config->flags.reset_active_high;
    // 初始化面板对象的函数指针。
    st7789v->base.del = panel_st7789v_del;
    st7789v->base.reset = panel_st7789v_reset;
    st7789v->base.init = panel_st7789v_init;
    st7789v->base.draw_bitmap = panel_st7789v_draw_bitmap;
    st7789v->base.invert_color = panel_st7789v_invert_color;
    st7789v->base.set_gap = panel_st7789v_set_gap;
    st7789v->base.mirror = panel_st7789v_mirror;
    st7789v->base.swap_xy = panel_st7789v_swap_xy;
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    st7789v->base.disp_off = panel_st7789v_disp_on_off;
#else
    st7789v->base.disp_on_off = panel_st7789v_disp_on_off;
#endif
    // 返回创建的面板对象。
    *ret_panel = &(st7789v->base);
    // 记录新创建的ST7789V面板对象的位置。
    ESP_LOGD(TAG, "new st7789v panel @%p", st7789v);

    return ESP_OK; // 操作成功，返回ESP_OK。

err: // 错误处理路径。
    // 如果已分配内存，则释放资源并重置GPIO状态。
    if (st7789v) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(st7789v);
    }
    // 返回错误码。
    return ret;
}

/**
 * @brief 释放ST7789V面板资源
 * 
 * 本函数用于释放与ST7789V液晶面板相关的资源。当不再需要使用面板时，可以通过此函数进行资源清理。
 * 主要包括重置GPIO引脚状态和释放面板数据结构的内存。
 * 
 * @param panel 液晶面板句柄，指向具体的st7789v_panel_t结构
 * @return esp_err_t 返回操作状态，ESP_OK表示成功
 */
static esp_err_t panel_st7789v_del(esp_lcd_panel_t *panel)
{
    // 将base转换回st7789v_panel_t类型，以访问特定于ST7789V面板的成员
    st7789v_panel_t *st7789v = __containerof(panel, st7789v_panel_t, base);

    // 如果重置GPIO引脚编号有效，则重置该引脚状态
    if (st7789v->reset_gpio_num >= 0) {
        gpio_reset_pin(st7789v->reset_gpio_num);
    }

    // 记录面板释放操作的日志信息
    ESP_LOGD(TAG, "del st7789v panel @%p", st7789v);

    // 释放面板数据结构占用的内存
    free(st7789v);

    // 返回操作状态，表示释放资源成功
    return ESP_OK;
}

/**
 * 对ST7789V液晶面板进行复位操作
 * 该函数根据具体情况选择是进行硬件复位还是软件复位
 * 
 * @param panel 液晶面板的句柄，用于获取ST7789V特定的面板操作结构体
 * @return esp_err_t 返回操作结果，ESP_OK表示成功
 */
static esp_err_t panel_st7789v_reset(esp_lcd_panel_t *panel)
{
    // 将传入的通用面板句柄转换为ST7789V特定的面板操作结构体
    st7789v_panel_t *st7789v = __containerof(panel, st7789v_panel_t, base);
    // 获取ST7789V面板的IO处理句柄
    esp_lcd_panel_io_handle_t io = st7789v->io;

    // 根据reset_gpio_num的值判断是进行硬件复位还是软件复位
    if (st7789v->reset_gpio_num >= 0) {
        // 进行硬件复位
        // 首先设置复位引脚为高电平（或低电平，根据reset_level的值）
        gpio_set_level(st7789v->reset_gpio_num, st7789v->reset_level);
        // 延时10ms
        vTaskDelay(pdMS_TO_TICKS(10));
        // 将复位引脚电平翻转，完成复位操作
        gpio_set_level(st7789v->reset_gpio_num, !st7789v->reset_level);
        // 再次延时10ms，确保复位有效
        vTaskDelay(pdMS_TO_TICKS(10));
    } else {
        // 进行软件复位
        // 通过IO接口发送软件复位命令
        esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0);
        // 根据规范，发送新命令前至少等待5ms
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // 返回操作成功标志
    return ESP_OK;
}

typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t data_bytes; // Length of data in above data array; 0xFF = end of cmds.
} lcd_init_cmd_t;

static const lcd_init_cmd_t vendor_specific_init[] = {
    /* Memory Data Access Control, MX=MV=1, MY=ML=MH=0, RGB=0 */
    {0x36, {0x00}, 1},
    /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Porch Setting */
    {0xB2, {0x0c, 0x0c, 0x00, 0x33, 0x33}, 5},
    /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
    {0xB7, {0x45}, 1},
    /* VCOM Setting, VCOM=1.175V */
    {0xBB, {0x2B}, 1},
    /* LCM Control, XOR: BGR, MX, MH */
    {0xC0, {0x2C}, 1},
    /* VDV and VRH Command Enable, enable=1 */
    {0xC2, {0x01, 0xff}, 2},
    /* VRH Set, Vap=4.4+... */
    {0xC3, {0x11}, 1},
    /* VDV Set, VDV=0 */
    {0xC4, {0x20}, 1},
    /* Frame Rate Control, 60Hz, inversion=0 */
    {0xC6, {0x0f}, 1},
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
    {0xD0, {0xA4, 0xA1}, 1},
    /* Positive Voltage Gamma Control */
    {0xE0, {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19}, 14},
    /* Negative Voltage Gamma Control */
    {0xE1, {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19}, 14},
    /* Sleep Out */
    {0x11, {0}, 0x80},
    /* Display On */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff}
};

/**
 * 初始化 ST7789V 面板。
 * 
 * 此函数负责初始化 ST7789V LCD 面板，包括设置基础参数和厂商特定的初始化。
 * 在显示模块的初始化过程中会被调用。
 * 
 * @param panel 指向 LCD 面板句柄的指针，用于操作 LCD 面板。
 * @return 返回初始化结果代码，ESP_OK 表示成功。
 */
static esp_err_t panel_st7789v_init(esp_lcd_panel_t *panel)
{
    // 将 LCD 面板句柄转换为 ST7789V 特定的句柄以便操作
    st7789v_panel_t *st7789v = __containerof(panel, st7789v_panel_t, base);
    // 获取 LCD 面板 IO 句柄以执行后续的 LCD 命令操作
    esp_lcd_panel_io_handle_t io = st7789v->io;

    // LCD 上电复位后默认进入睡眠模式并关闭显示，首先退出睡眠模式
    esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0);
    // 添加延时确保 LCD 已经退出睡眠模式
    vTaskDelay(pdMS_TO_TICKS(100));
    // 设置内存访问控制模式
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        st7789v->madctl_val,
    }, 1);
    // 设置颜色模式
    esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD, (uint8_t[]) {
        st7789v->colmod_cal,
    }, 1);

    // 厂商特定的初始化，不同厂商可能有不同的初始化序列
    // 应咨询 LCD 供应商获取具体的初始化序列代码
    int cmd = 0;
    // 遍历厂商特定的初始化数组，向 LCD 发送初始化命令和参数
    while (vendor_specific_init[cmd].data_bytes != 0xff) {
        esp_lcd_panel_io_tx_param(io, vendor_specific_init[cmd].cmd, vendor_specific_init[cmd].data, vendor_specific_init[cmd].data_bytes & 0x1F);
        cmd++;
    }

    // 初始化完成，返回结果代码
    return ESP_OK;
}

/**
 * @brief 在ST7789V面板上绘制位图
 * 
 * 此函数在指定的LCD面板上绘制位图。它首先验证起始坐标小于结束坐标，
 * 然后根据面板的间隙调整坐标，接着设置列地址和行地址，最后将位图数据写入帧缓冲区。
 * 
 * @param panel LCD面板驱动结构，用于访问IO操作和配置信息
 * @param x_start 绘制区域的起始X坐标
 * @param y_start 绘制区域的起始Y坐标
 * @param x_end 绘制区域的结束X坐标（不包含此坐标）
 * @param y_end 绘制区域的结束Y坐标（不包含此坐标）
 * @param color_data 位图数据指针，按行存储的颜色数据
 * 
 * @return esp_err_t 返回操作结果，ESP_OK表示成功
 */
static esp_err_t panel_st7789v_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    // 将base转换为st7789v_panel_t类型，以访问特定于ST7789V的成员
    st7789v_panel_t *st7789v = __containerof(panel, st7789v_panel_t, base);
    
    // 确保起始坐标小于结束坐标
    assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
    
    // 获取IO处理句柄
    esp_lcd_panel_io_handle_t io = st7789v->io;

    // 考虑到面板的边距，调整坐标
    x_start += st7789v->x_gap;
    x_end += st7789v->x_gap;
    y_start += st7789v->y_gap;
    y_end += st7789v->y_gap;

    // 设置列地址范围，定义MCU可以访问的帧内存区域
    esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, (uint8_t[]) {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        ((x_end - 1) >> 8) & 0xFF,
        (x_end - 1) & 0xFF,
    }, 4);
    
    // 设置行地址范围
    esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, (uint8_t[]) {
        (y_start >> 8) & 0xFF,
        y_start & 0xFF,
        ((y_end - 1) >> 8) & 0xFF,
        (y_end - 1) & 0xFF,
    }, 4);
    
    // 计算需要传输的数据长度，并写入颜色数据到帧缓冲区
    size_t len = (x_end - x_start) * (y_end - y_start) * st7789v->fb_bits_per_pixel / 8;
    esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, color_data, len);

    // 返回操作结果
    return ESP_OK;
}

/**
 * @brief ST7789V面板颜色反转设置函数
 * 
 * 此函数用于设置ST7789V液晶面板的颜色反转状态。颜色反转可以通过设置invert_color_data参数为true或false来实现。
 * 当invert_color_data为true时，启动颜色反转；当invert_color_data为false时，关闭颜色反转。
 * 
 * @param panel 液晶面板的句柄，用于标识具体的面板实例
 * @param invert_color_data 指示是否启用颜色反转的标志，true表示启用颜色反转，false表示禁用颜色反转
 * @return esp_err_t 返回操作状态，ESP_OK表示操作成功
 */
static esp_err_t panel_st7789v_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    // 将panel转换为ST7789V特定的面板类型指针
    st7789v_panel_t *st7789v = __containerof(panel, st7789v_panel_t, base);
    // 获取IO操作句柄，用于与面板进行通信
    esp_lcd_panel_io_handle_t io = st7789v->io;
    // 定义一个变量用于存储LCD命令
    int command = 0;

    // 根据invert_color_data的值选择合适的LCD命令
    if (invert_color_data) {
        command = LCD_CMD_INVON;
    } else {
        command = LCD_CMD_INVOFF;
    }

    // 通过IO接口发送命令及参数给LCD面板，以应用颜色反转设置
    esp_lcd_panel_io_tx_param(io, command, NULL, 0);

    // 返回操作状态，表示颜色反转设置完成
    return ESP_OK;
}

/**
 * 配置ST7789V液晶面板的镜像显示模式
 * 
 * @param panel 面板控制对象
 * @param mirror_x 是否在X轴上镜像显示
 * @param mirror_y 是否在Y轴上镜像显示
 * 
 * @return 操作结果，ESP_OK表示成功
 */
static esp_err_t panel_st7789v_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    // 将panel转换为特定的ST7789V面板类型
    st7789v_panel_t *st7789v = __containerof(panel, st7789v_panel_t, base);
    // 获取IO句柄，用于与面板通信
    esp_lcd_panel_io_handle_t io = st7789v->io;

    // 根据mirror_x参数设置X轴镜像标志位
    if (mirror_x) {
        st7789v->madctl_val |= LCD_CMD_MX_BIT;
    } else {
        st7789v->madctl_val &= ~LCD_CMD_MX_BIT;
    }

    // 根据mirror_y参数设置Y轴镜像标志位
    if (mirror_y) {
        st7789v->madctl_val |= LCD_CMD_MY_BIT;
    } else {
        st7789v->madctl_val &= ~LCD_CMD_MY_BIT;
    }

    // 向面板发送MADCTL命令，配置镜像模式
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        st7789v->madctl_val
    }, 1);

    // 返回操作成功标志
    return ESP_OK;
}

/**
 * @brief 交换LCD面板的X和Y轴
 * 
 * 此函数通过修改MADCTL寄存器的设置来交换ST7789V LCD面板的X和Y轴。
 * 它是静态函数，仅在当前文件中使用。
 * 
 * @param panel LCD面板的句柄，实际上是st7789v_panel_t的指针
 * @param swap_axes 表示是否交换XY轴的标志，true为交换，false为不交换
 * @return esp_err_t 返回操作结果，ESP_OK表示成功
 */
static esp_err_t panel_st7789v_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    // 将panel转换为st7789v_panel_t的指针，以便访问特定于ST7789V的成员
    st7789v_panel_t *st7789v = __containerof(panel, st7789v_panel_t, base);
    // 获取LCD面板的IO句柄，用于与面板通信
    esp_lcd_panel_io_handle_t io = st7789v->io;
    
    // 根据swap_axes的值设置或清除MADCTL寄存器中的MV位
    if (swap_axes) {
        st7789v->madctl_val |= LCD_CMD_MV_BIT;
    } else {
        st7789v->madctl_val &= ~LCD_CMD_MV_BIT;
    }
    
    // 通过IO接口向LCD面板发送MADCTL命令和修改后的值
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        st7789v->madctl_val
    }, 1);
    
    // 返回操作成功标志
    return ESP_OK;
}

/**
 * @brief 设置ST7789V液晶面板的X和Y方向的间隙
 * 
 * 该函数用于配置ST7789V液晶面板的X和Y方向的间隙，这些间隙可能用于特定的显示布局需求。
 * 
 * @param panel 液晶面板句柄，实际上是一个指向st7789v_panel_t结构体的指针
 * @param x_gap X方向的间隙大小
 * @param y_gap Y方向的间隙大小
 * 
 * @return esp_err_t 返回操作状态，ESP_OK表示设置成功
 */
static esp_err_t panel_st7789v_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    // 将传入的panel参数转换为st7789v_panel_t*类型，以便访问ST7789V特定的成员变量
    st7789v_panel_t *st7789v = __containerof(panel, st7789v_panel_t, base);
    // 设置X方向的间隙大小
    st7789v->x_gap = x_gap;
    // 设置Y方向的间隙大小
    st7789v->y_gap = y_gap;
    // 返回操作成功状态
    return ESP_OK;
}

/**
 * @brief ST7789V液晶面板电源开关控制函数
 * 
 * 此函数用于控制ST7789V液晶面板的电源开关。根据传入的on_off参数，
 * 函数将向液晶面板发送相应的指令，以打开或关闭电源。
 * 
 * @param panel 液晶面板的描述符，用于识别特定的液晶面板
 * @param on_off 指示电源开关状态的布尔值；true为打开电源，false为关闭电源
 * @return esp_err_t 返回操作状态，ESP_OK表示操作成功
 */
static esp_err_t panel_st7789v_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    // 将panel转换为特定的ST7789V面板描述符
    st7789v_panel_t *st7789v = __containerof(panel, st7789v_panel_t, base);
    // 获取IO操作句柄，用于与液晶面板进行通信
    esp_lcd_panel_io_handle_t io = st7789v->io;
    // 初始化指令值
    int command = 0;

    // 对于旧版本的IDF（小于5.0.0），需要反转on_off的值
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    on_off = !on_off;
#endif

    // 根据on_off参数选择合适的LCD命令
    if (on_off) {
        command = LCD_CMD_DISPON;
    } else {
        command = LCD_CMD_DISPOFF;
    }
    // 向液晶面板发送命令和参数
    esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    // 返回操作状态
    return ESP_OK;
}
