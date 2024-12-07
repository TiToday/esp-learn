#ifndef _ST7789_DRIVER_H_
#define _ST7789_DRIVER_H_
#include <esp_lcd_types.h>
#include <esp_lcd_panel_vendor.h>

esp_err_t esp_lcd_new_panel_st7789v(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

#endif