#include "aliot_dm.h"
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>


static int s_aliot_id = 0;
//创建物模型描述结构体
ALIOT_DM_DES *aliot_malloc_des(void)
{
    ALIOT_DM_DES *dm = (ALIOT_DM_DES*)malloc(sizeof(ALIOT_DM_DES));
    if(dm)
    {
        memset(dm,0,sizeof(ALIOT_DM_DES));
        dm->dm_js = cJSON_CreateObject();

        char id[10];
        snprintf(id,sizeof(id),"%d",s_aliot_id);
        cJSON_AddStringToObject(dm->dm_js,"id",id);
        cJSON_AddStringToObject(dm->dm_js,"version","1.0");
        cJSON_AddStringToObject(dm->dm_js,"method","thing.event.property.post");
        cJSON_AddObjectToObject(dm->dm_js,"params");
        return dm;
    }
    return NULL;

}


//设置结构体属性值
void aliot_set_dm_int(ALIOT_DM_DES *dm,const char* name,int value)
{
    if(dm)
    {
        cJSON *paragms_js = cJSON_GetObjectItem(dm->dm_js,"params");
        if(paragms_js)
        {
            cJSON_AddNumberToObject(paragms_js,name,value);
        }
    }


}

//生成json字符串,保存在dm_js_str
void aliot_dm_serialize(ALIOT_DM_DES *dm)
{
    if(dm)
    {
        if(dm->dm_js_str)
        {
            cJSON_free(dm->dm_js_str);
            dm->dm_js_str = NULL;
        }

        dm->dm_js_str = cJSON_PrintUnformatted(dm->dm_js);
    }

}

//释放结构体
void aliot_dm_free(ALIOT_DM_DES *dm)
{
    if(dm)
    {
        if(dm->dm_js_str)
        {
            cJSON_free(dm->dm_js_str);
            dm->dm_js_str = NULL;
        }

        if(dm->dm_js)
        {
            cJSON_Delete(dm->dm_js);
            dm->dm_js = NULL;
        }
        free(dm);
    }

}