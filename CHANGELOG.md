项目的日常修改和显著变更都将记录在文件中。
由于本人技术有限，日志规范持续优化中。
***
# 2026-3-24
* 完成了项目的空项目模板的构建，为之后的开发做好准备。
* 准备之后先在开发板上运行freeRTOS
* 在freeRTOS的基础上实现和运行功能
***
# 2026-3-25
* 为了方便后续开发GUI来完善整个项目在项目添加了LVGL9.2.2组件。
* LVGL中自带了Cmakelists.txt和kconfig，通过重新运行set-target esp32s3指令即可将LVGL添加到menuconfig中。
* 学习了开发日志的编写规范并修改了README文件。
* 后续将会尝试学习Keep a Changelog日志编写风格。
***
# Changelog
格式基于 [Keep a Changelog]

## [Unreleased]

### 2026-3-25

#### Added
- 为CHANGELOG添加了更加规范的日志编写。
- 为项目添加了LVGL9.2.2。

#### Changed
- 将README中的日志部分转移到CHANGELOG中。

### 2026-3-30

#### Added
- 为项目添加了managed_components。
- 为项目添加了lvgl__lvgl。
- 为项目添加了esp_lvgl_port。
- 为项目添加了esp_lcd_st7796。

#### Removed
- 移除LVGL9.2.2所有组件。
- 移除所有基于LVGL9.2.2编写的驱动。
  
#### Changed
- 修改开发所使用的LVGL版本为9.5.0。
- 修改了BLE_LED.c以实现用ESP32S3驱动LCD屏幕。

#### Achieved
- 实现了使用ESP32S3驱动LCD屏幕。