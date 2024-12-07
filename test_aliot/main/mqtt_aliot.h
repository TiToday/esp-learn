#ifndef _MQTT_ALIOT_H_
#define _MQTT_ALIOT_H_

//设备名称
#define ALIOT_DEVICENAME "esp32-RGB01"
//产品KEY
#define ALIOT_PRODUCTKEY "k1xpiUduCK8"
//设备密钥
#define ALIOT_DEVICESECRET "cce1ba311d6b51f04e97582508613bd0"
//MQTT域名
#define ALIOT_MQTT_URL "mqtts://iot-06z00dgoeyq5bib.mqtt.iothub.aliyuncs.com"

void mqtt_aliot_init(void);

//上报一个整形数据
void aliot_post_prorerty_int(const char *name, int value);

//是否连接阿里云
char is_mqtt_connected(void);

#endif
