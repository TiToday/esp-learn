#ifndef _LV_PORT_H_
#define _LV_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "st7789_driver.h"
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define LCD_HOST SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ (60 * 1000 * 1000)
#define PIN_NUM_SCLK 19
#define PIN_NUM_MOSI 18
#define PIN_NUM_MISO -1
#define PIN_NUM_LCD_DC 0
#define PIN_NUM_LCD_RST 1
#define PIN_NUM_LCD_CS 23
// #define PIN_NUM_TOUCH_CS -1 // 测试

#define PIN_NUM_BK_LIGHT 3
#define LCD_BK_LIGHT_ON_LEVEL 1
#define LCD_BK_LIGHT_OFF_LEVEL !LCD_BK_LIGHT_ON_LEVEL

#define I2C_HOST 0
#define CST816T_SENSOR_ADDR 0x15
#define I2C_MASTER_SCL_IO 7
#define I2C_MASTER_SDA_IO 6
#define I2C_MASTER_FREQ_HZ 600000
#define I2C_MASTER_TIMEOUT_MS 10

#define FingerNum 0x02
#define XposH 0x03

#define LCD_H_RES 240
#define LCD_V_RES 280

// Bit number used to represent command and parameter
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8

#define LVGL_TICK_PERIOD_MS 2
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void e53_lcd_init(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif 