#ifndef _ALIOT_DM_H_
#define _ALIOT_DM_H_
#include "cJSON.h"

typedef struct
{
    cJSON* dm_js;
    char* dm_js_str;
}ALIOT_DM_DES;

//创建物模型描述结构体
ALIOT_DM_DES *aliot_malloc_des(void);
//设置结构体属性值
void aliot_set_dm_int(ALIOT_DM_DES *dm,const char* name,int value);
//生成json字符串,保存在dm_js_str
void aliot_dm_serialize(ALIOT_DM_DES *dm);
//释放结构体
void aliot_dm_free(ALIOT_DM_DES *dm);

#if 0
ALIOT_DM_DES* dm  = aliot_malloc_des();
aliot_set_dm_int(dm,"LightSwitch",1);
aliot_dm_serialize(dm);
dm->dm_js_str;

#endif

#endif