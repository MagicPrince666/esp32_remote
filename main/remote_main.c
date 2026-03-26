#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "common.h"
#ifdef USE_LVGL_DISPLAY
#include "lvgl_demo_ui.h"
#else
#include "lcd_driver.h"
#endif

void app_main(void)
{
#ifdef USE_LVGL_DISPLAY
    InitLvgl();
#else
    InitLCD();
#endif
}
