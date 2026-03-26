#ifndef __COMMON_H__
#define __COMMON_H__

#if CONFIG_EXAMPLE_LCD_CONTROLLER_ILI9341
#include "esp_lcd_ili9341.h"
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_GC9A01
#include "esp_lcd_gc9a01.h"
#endif

#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_STMPE610
#include "esp_lcd_touch_stmpe610.h"
#elif CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_XPT2046
#include "esp_lcd_touch_xpt2046.h"
#endif

// Using SPI2 in the example
#define LCD_HOST  SPI2_HOST

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  GPIO_NUM_NC
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_SCLK           GPIO_NUM_16
#define EXAMPLE_PIN_NUM_MOSI           GPIO_NUM_17
#define EXAMPLE_PIN_NUM_MISO           GPIO_NUM_21
#define EXAMPLE_PIN_NUM_LCD_DC         GPIO_NUM_26
#define EXAMPLE_PIN_NUM_LCD_RST        GPIO_NUM_NC
#define EXAMPLE_PIN_NUM_LCD_CS         GPIO_NUM_25
#define EXAMPLE_PIN_NUM_BK_LIGHT       GPIO_NUM_27
#define EXAMPLE_PIN_NUM_TOUCH_CS       GPIO_NUM_22
#define EXAMPLE_PIN_NUM_TOUCH_INT      GPIO_NUM_33

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              240
#define EXAMPLE_LCD_V_RES              320
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_LVGL_DRAW_BUF_LINES    20 // number of display lines in each draw buffer
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1000 / CONFIG_FREERTOS_HZ
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (4 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2
#define LCD_BK_LIGHT_ON_LEVEL   1
#define PARALLEL_LINES 16

#endif /* __COMMON_H__ */
