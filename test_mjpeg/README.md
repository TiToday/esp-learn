# mjpeg

#### 介绍
本仓库是使用ESP32和LVGL来播放mjpeg格式的视频，采用ESP-IDF v5.2版本框架

#### 软件架构
软件架构说明


#### 安装教程
源码下载方式，推荐使用git工具下载
git clone --recursive https://gitee.com/vi-iot/mjpeg.git


#### 使用说明
例程配套的开发板链接如下：
https://item.taobao.com/item.htm?ft=t&id=802401650392&spm=a21dvs.23580594.0.0.4fee645eXpkfcp&skuId=5635015963649

可使用ffmpeg工具，将mp4格式的视频转为mjpeg格式，转化命令如下

ffmpeg -i input.mp4 -c:v mjpeg -qscale:v 2 -vf "scale=-2:120" output.mjpeg
上述命令中 input.mp4 是你要转换的视频，output.mjpeg 是转化后的视频，120表示视频高度为120，宽度按比例保持

本工程需要使用MicroSD卡，将转化后的视频通过读卡器放在MicroSD卡中，然后将MicroSD卡插到开发板后，程序可以识别出文件列表



