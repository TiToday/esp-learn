idf_component_register(
    SRCS "st7789_driver.c" "led_ws2812.c" "dht11.c"
    INCLUDE_DIRS "."
    REQUIRES esp_lcd
)