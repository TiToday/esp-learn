#include "lv_port.h"

static const char *TAG = "E53_LCD";

extern void lvgl_demo_ui(lv_disp_t *disp);
uint8_t cst816t_read_len(uint16_t reg_addr, uint8_t *data, uint8_t len);
static esp_err_t testtouch(uint8_t *touch_points_num);
esp_err_t get_coordinates(uint16_t *x, uint16_t *y);
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
static void lvgl_port_update_callback(lv_disp_drv_t *drv);
static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);
static void increase_lvgl_tick(void *arg);

/**
 * 从CST816T传感器读取指定长度的数据
 *
 * @param reg_addr 寄存器地址，标识读取数据的起始位置
 * @param data 接收数据的缓冲区指针
 * @param len 指定要读取的数据长度
 *
 * @return 返回实际读取到的数据长度，成功返回0，失败返回错误码
 *
 * 此函数用于通过I2C总线从CST816T传感器的指定寄存器地址开始读取len长度的数据，
 * 并将读取到的数据存储在data指向的缓冲区中。使用了I2C主机模式下的写读操作，
 * 其中写操作仅包含寄存器地址，读操作则读取len字节的数据。在超时时间内等待I2C操作完成。
 */
uint8_t cst816t_read_len(uint16_t reg_addr, uint8_t *data, uint8_t len)
{
    uint8_t res = 0;
    res = i2c_master_write_read_device(I2C_HOST, CST816T_SENSOR_ADDR, (uint8_t *)&reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    return res;
}

/**
 * @brief 读取触摸屏上的触摸点数量
 *
 * 本函数通过调用cst816t_read_len函数，从FingerNum寄存器中读取触摸点的数量，并将结果存储在touch_points_num指向的变量中。
 *
 * @param touch_points_num 指向用于存储触摸点数量的变量的指针
 * @return esp_err_t 返回读取操作的结果，0表示成功，非0表示失败
 */
static esp_err_t testtouch(uint8_t *touch_points_num)
{
    uint8_t res = 0;                                        // 初始化返回结果变量为0
    res = cst816t_read_len(FingerNum, touch_points_num, 1); // 从FingerNum寄存器中读取触摸点数量
    return res;                                             // 返回读取操作的结果
}

/**
 * 获取坐标值
 * 该函数从触摸屏控制器中读取坐标数据，并将其解析为 x 和 y 坐标值
 *
 * @param x 指向接收解析后的 x 坐标值的变量的指针
 * @param y 指向接收解析后的 y 坐标值的变量的指针
 * @return 返回 ESP_OK，表示操作成功
 *
 * 说明：
 * - 该函数首先从触摸屏控制器读取 4 个字节的数据，这些数据用于解析坐标值
 * - 数据的解析过程涉及位操作和移位，以从读取的数据中提取坐标信息
 * - 解析后的坐标值被检查是否在指定的范围内（x <= 240 和 y <= 280），如果在范围内，则将值赋予 x 和 y 参数
 * - 无论坐标值是否在范围内，函数都返回 ESP_OK，表示执行成功
 */
esp_err_t get_coordinates(uint16_t *x, uint16_t *y)
{
    // 定义一个 4 字节的数组，用于存储从触摸屏控制器读取的数据
    uint8_t data[4];
    // 定义两个整数变量，用于解析和存储解析后的坐标值
    int a, b;

    // 使用静态变量来存储上一次的坐标值
    static int last_x = 0;
    static int last_y = 0;

    // 从触摸屏控制器读取 4 个字节的数据，以获取坐标信息
    cst816t_read_len(XposH, data, 4);

    //默认角度

    // 解析数据以获取坐标值 a 和 b
    a = ((data[0] & 0x0f) << 8) | data[1];
    b = ((data[2] & 0x0f) << 8) | data[3];

    // 检查解析后的坐标值是否在指定的范围内
    if ((a <= 240) && (b <= 280))
    {
        // 如果在范围内，将坐标值赋予 x 和 y 参数
        *x = a;
        *y = b;

        // 检查当前坐标是否与上次坐标不同
        if (*x != last_x || *y != last_y)
        {
            // 更新上一次坐标值
            last_x = *x;
            last_y = *y;

            // 记录日志
            ESP_LOGI(TAG, "X=%d Y=%d", *x, *y);
        }
    }
    
    // 返回 ESP_OK，表示函数执行成功
    return ESP_OK;
}

/**
 * 通知LVGL刷新准备就绪
 *
 * 该函数是一个回调函数，用于在LCD面板IO事件中通知LVGL刷新准备就绪它通过传入的LCD面板IO句柄和事件数据，
 * 使用用户上下文中的LVGL显示驱动句柄调用LVGL的刷新准备就绪函数
 *
 * @param panel_io LCD面板IO句柄，标识触发事件的LCD面板IO
 * @param edata 指向LCD面板IO事件数据的指针，包含事件相关信息
 * @param user_ctx 用户上下文数据，此处为LVGL显示驱动句柄的指针
 *
 * @return 总是返回false，表示该事件处理函数仅用于通知刷新准备就绪，不进行进一步处理
 */
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    // 将用户上下文转换为LVGL显示驱动句柄指针
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;

    // 调用LVGL的刷新准备就绪函数，通知LVGL刷新操作可以开始
    lv_disp_flush_ready(disp_driver);

    // 返回false以指示该事件处理函数的用途，并避免进一步处理
    return false;
}

/**
 * @brief LVGL显示驱动程序的刷新回调函数
 *
 * 本函数作为LVGL显示驱动的一部分，负责将LVGL缓冲区的内容刷新到实际的显示面板上。
 * 根据旋转状态，适当调整绘图位置，确保图像正确显示。
 *
 * @param drv LVGL显示驱动结构指针，包含刷新所需的信息
 * @param area 需要刷新的屏幕区域
 * @param color_map 包含需要刷新的像素颜色数据的缓冲区
 */
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    // 获取存储在drv->user_data中的LCD面板句柄
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    // 初始化区域坐标变量
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;

    // 根据屏幕的旋转状态选择正确的绘图位置
    switch (drv->rotated)
    {
    case LV_DISP_ROT_NONE: // 无旋转
        // 直接绘制位图，不调整坐标
        esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1 + 20, offsetx2 + 1, offsety2 + 21, color_map);
        break;
    case LV_DISP_ROT_90: // 旋转90度
        // 调整坐标，将位图旋转90度后绘制
        esp_lcd_panel_draw_bitmap(panel_handle, offsetx1 + 20, offsety1, offsetx2 + 21, offsety2 + 1, color_map);
        break;
    case LV_DISP_ROT_180: // 旋转180度
        // 直接绘制位图，不调整坐标，与无旋转相同
        esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1 + 20, offsetx2 + 1, offsety2 + 21, color_map);
        break;
    case LV_DISP_ROT_270: // 旋转270度
        // 调整坐标，将位图旋转270度后绘制
        esp_lcd_panel_draw_bitmap(panel_handle, offsetx1 + 20, offsety1, offsetx2 + 21, offsety2 + 1, color_map);
        break;
    }
}

/* Rotate display and touch, when rotated screen in LVGL. Called when driver parameters are updated. */
/**
 * @brief LVGL端口更新回调函数
 *
 * 该函数根据LVGL显示驱动程序的旋转设置更新LCD面板的旋转状态。通过修改LCD面板的坐标轴和镜像设置，
 * 实现对LVGL显示旋转的硬件支持。
 *
 * @param drv LVGL显示驱动程序指针，包含旋转状态和用户数据（LCD面板句柄）。
 */
static void lvgl_port_update_callback(lv_disp_drv_t *drv)
{
    // 获取LCD面板句柄
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;

    // 根据LVGL的旋转设置更新LCD面板的旋转状态
    switch (drv->rotated)
    {
    case LV_DISP_ROT_NONE:
        // 无旋转
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, false);
        break;
    case LV_DISP_ROT_90:
        // 旋转90度
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, true);
        break;
    case LV_DISP_ROT_180:
        // 旋转180度
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, true);
        break;
    case LV_DISP_ROT_270:
        // 旋转270度
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, false);
        break;
    }
}

/**
 * LVGL触摸回调函数
 *
 * 该函数用作LVGL的输入设备驱动程序的一部分，用于处理触摸事件
 * 当有触摸事件发生时，该函数被调用，用于更新输入设备的状态
 *
 * @param drv 输入设备驱动程序指针，LVGL使用该指针来提供输入设备的状态信息
 * @param data 输入设备数据指针，LVGL使用该指针来获取触摸坐标和状态
 */
static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    // 初始化触摸坐标
    uint16_t touchpad_x = 0;
    uint16_t touchpad_y = 0;
    // 初始化触摸点数量
    uint8_t touch_points_num = 0;
    // 检测触摸点数量
    testtouch(&touch_points_num);

    // 当只有一个触摸点时，认为是有效的触摸事件
    if (touch_points_num == 1)
    {
        // 获取触摸坐标
        get_coordinates(&touchpad_x, &touchpad_y);
        // 确保获取的坐标非零
        if ((touchpad_x != 0) && (touchpad_y != 0))
        {
            // 更新LVGL输入设备数据结构中的触摸坐标
            data->point.x = touchpad_x;
            data->point.y = touchpad_y;
        }
        // 设置输入设备状态为按下
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        // 设置输入设备状态为释放，用于处理非触摸事件
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/**
 * increase_lvgl_tick - 定期调用以更新LVGL的系统节拍
 * @arg: 未使用，提供给定时器回调的参数（如果有的话）
 *
 * 该函数通过调用lv_tick_inc()来增加LVGL的系统节拍计数器。
 * 它告诉LVGL自上次调用以来已经过去了多少毫秒，用于LVGL内部的时间管理。
 */
static void increase_lvgl_tick(void *arg)
{
    /* 增加LVGL的系统节拍，参数为每次更新的毫秒数 */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

void e53_lcd_init(void)
{
    // 静态变量disp_buf，包含内部图形缓冲区，称为绘制缓冲区
    static lv_disp_draw_buf_t disp_buf;
    // 静态变量disp_drv，包含回调函数
    static lv_disp_drv_t disp_drv;

    // 关闭LCD背光
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT};
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    // 初始化SPI总线
    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t spi_buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_V_RES * LCD_H_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &spi_buscfg, SPI_DMA_CH_AUTO));

    // 安装面板IO
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
    };
    // 将LCD连接到SPI总线
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    // 安装ST7789V面板驱动
    ESP_LOGI(TAG, "Install ST7789V panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789v(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    // 在打开屏幕或背光之前，用户可以将预定义的图案刷新到屏幕上
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // 打开LCD背光
    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);

    // 初始化I2C总线
    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config((i2c_port_t)I2C_HOST, &conf));
    ESP_ERROR_CHECK(i2c_driver_install((i2c_port_t)I2C_HOST, conf.mode, 0, 0, 0));

    // 初始化LVGL库
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    // 分配LVGL使用的绘制缓冲区
    // 建议绘制缓冲区的大小至少为屏幕大小的1/10
    lv_color_t *buf1 = heap_caps_malloc(LCD_V_RES * LCD_H_RES * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(LCD_V_RES * LCD_H_RES * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    // 初始化LVGL绘制缓冲区
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_V_RES * LCD_H_RES);

    // 向LVGL注册显示驱动
    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);

    // 设置显示驱动的旋转状态为90度
    disp_drv.rotated = LV_DISP_ROT_270;

    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.drv_update_cb = lvgl_port_update_callback;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    // 交换面板的X和Y轴
    esp_lcd_panel_swap_xy(panel_handle, true);
    // 镜像面板的X和Y轴
    esp_lcd_panel_mirror(panel_handle, true, false);

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // 用于LVGL的Tick接口（使用esp_timer生成2ms周期事件）
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    // 初始化输入设备驱动
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    // 设置输入设备类型为指针类型
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    // 设置输入设备的显示器为disp
    indev_drv.disp = disp;
    // 设置输入设备的读取回调函数为lvgl_touch_cb
    indev_drv.read_cb = lvgl_touch_cb;
    // 设置输入设备的用户数据为tp
    // indev_drv.user_data = tp;

    // 注册输入设备驱动
    lv_indev_drv_register(&indev_drv);

    ESP_LOGI(TAG, "Display LVGL");
}
