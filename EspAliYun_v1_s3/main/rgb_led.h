#ifndef _RGB_LED_H__
#define _RGB_LED_H__

typedef struct
{
    int led_switch;
    int red, green, blue;
} rgb_led_t;

typedef struct
{
    int wind_speed;        // 风速
    int air_temperature;   // 空气温度
    int air_humidity;      // 空气湿度
    int illuminance;       // 光照度
    int pressure;          // 气压
    int noise;             // 噪声
    int soil_moisture;     // 土壤水份
    int soil_temperature;  // 土壤温度
    int soil_conductivity; // 土壤电导率
    int soil_PH;           // 土壤PH
    int pump_switch;       // 泵开关
} environment_variable_t;

void rgb_led_init(void);
void rgb_led_update(void);
// void rgb_led_update(int red, int green, int blue);

#endif
