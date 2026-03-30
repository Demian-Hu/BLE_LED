#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7796.h"
#include "esp_lvgl_port.h"

// ========== 根据你的硬件修改这些引脚定义 ==========
#define LCD_SPI_HOST        SPI3_HOST
#define LCD_GPIO_SCLK       GPIO_NUM_12
#define LCD_GPIO_MOSI       GPIO_NUM_11
#define LCD_GPIO_MISO       GPIO_NUM_13
#define LCD_GPIO_DC         GPIO_NUM_9
#define LCD_GPIO_CS         GPIO_NUM_10
#define LCD_GPIO_RST        GPIO_NUM_8   // 改为实际 RST 引脚
#define LCD_GPIO_BL         GPIO_NUM_45

// 屏幕参数 - ST7796 常见分辨率
#define LCD_H_RES           320
#define LCD_V_RES           480
#define LCD_DRAW_BUFF_HEIGHT 20

static const char *TAG = "LCD_TEST";

static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;
static lv_display_t *lvgl_disp = NULL;

static esp_err_t init_spi_bus(void)
{
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_GPIO_SCLK,
        .mosi_io_num = LCD_GPIO_MOSI,
        .miso_io_num = GPIO_NUM_NC,  // ST7796 不需要 MISO
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_H_RES * LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
    };
    return spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
}

static esp_err_t init_lcd_panel(void)
{
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_GPIO_DC,
        .cs_gpio_num = LCD_GPIO_CS,
        .pclk_hz = 20 * 1000 * 1000,  // 降低到 20MHz 测试
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &lcd_io),
        TAG, "New panel IO failed"
    );

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_GPIO_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,  // ST7796 通常是 BGR
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_st7796(lcd_io, &panel_config, &lcd_panel),
        TAG, "New panel failed"
    );

    // 复位和初始化
    esp_lcd_panel_reset(lcd_panel);
    vTaskDelay(pdMS_TO_TICKS(100));  // 等待复位完成
    
    esp_lcd_panel_init(lcd_panel);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 关闭镜像，先测试原始方向
    esp_lcd_panel_mirror(lcd_panel, false, true);
    
    // 设置颜色反转（ST7796 可能需要）
    esp_lcd_panel_invert_color(lcd_panel, false);
    
    // 开启显示
    esp_lcd_panel_disp_on_off(lcd_panel, true);
    
    return ESP_OK;
}

static esp_err_t init_lvgl(void)
{
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 8192,  // 增加栈空间
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5,
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL init failed");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = LCD_H_RES * LCD_DRAW_BUFF_HEIGHT,
        .double_buffer = false,  // 先关闭双缓冲测试
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = true,  // 先关闭镜像测试
        },
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
        },
    };
    
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting...");
    
    // 先开启背光，确保能看到画面
    gpio_set_direction(LCD_GPIO_BL, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_GPIO_BL, 1);
    ESP_LOGI(TAG, "Backlight ON");
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_ERROR_CHECK(init_spi_bus());
    ESP_LOGI(TAG, "SPI initialized");
    
    ESP_ERROR_CHECK(init_lcd_panel());
    ESP_LOGI(TAG, "LCD panel initialized");
    
    // 测试：直接填充屏幕颜色（绕过 LVGL）
    uint16_t *test_buf = heap_caps_malloc(LCD_H_RES * 100 * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (test_buf) {
        // 填充红色
        for (int i = 0; i < LCD_H_RES * 100; i++) {
            test_buf[i] = 0xF800;  // RGB565 红色
        }
        esp_lcd_panel_draw_bitmap(lcd_panel, 0, 0, LCD_H_RES, 100, test_buf);
        ESP_LOGI(TAG, "Red test pattern drawn");
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // 填充绿色
        for (int i = 0; i < LCD_H_RES * 100; i++) {
            test_buf[i] = 0x07E0;  // RGB565 绿色
        }
        esp_lcd_panel_draw_bitmap(lcd_panel, 0, 100, LCD_H_RES, 200, test_buf);
        ESP_LOGI(TAG, "Green test pattern drawn");
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // 填充蓝色
        for (int i = 0; i < LCD_H_RES * 100; i++) {
            test_buf[i] = 0x001F;  // RGB565 蓝色
        }
        esp_lcd_panel_draw_bitmap(lcd_panel, 0, 200, LCD_H_RES, 300, test_buf);
        ESP_LOGI(TAG, "Blue test pattern drawn");
        
        free(test_buf);
    }
    
    ESP_ERROR_CHECK(init_lvgl());
    ESP_LOGI(TAG, "LVGL initialized");
    
    // 创建 UI
    lvgl_port_lock(0);
    
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Hello ESP32-S3!");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(label);
    
    lvgl_port_unlock();
    
    ESP_LOGI(TAG, "Setup complete");
}