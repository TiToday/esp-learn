#ifndef _SDCARD_FILE_H_
#define _SDCARD_FILE_H_

#include "esp_err.h"

esp_err_t spiffs_init(char *partition_label, char *mount_point);
int spiffs_filelist(const char (**file)[256]);

#endif