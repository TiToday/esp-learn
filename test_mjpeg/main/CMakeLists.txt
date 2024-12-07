idf_component_register(SRCS "mjpeg_frame.c" "mjpeg.c" "lv_port.c" "sdcard_file.c" "ui_mjpeg_player.c"
                    INCLUDE_DIRS ".")

spiffs_create_partition_image(ui_img ../img FLASH_IN_PROJECT)
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)