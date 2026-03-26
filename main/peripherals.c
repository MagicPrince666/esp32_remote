#include "peripherals.h"
#include "rocker.h"
#include "pwm_ctrl.h"
#include "softap_sta.h"
#include "battery.h"
#include "select.h"
#include "serial.h"

#ifdef USE_LVGL_DISPLAY
static _lock_t *g_lvgl_mux = NULL;
static lv_obj_t * title;
static lv_obj_t * adc_chanals[5];
static lv_obj_t * battery_label;  // 电池电压标签
static lv_obj_t * battery_bar;    // 电池电量条
#endif
// const uint16_t adc_range[5] = {4095, 4095, 3821, 3740, 4095}; // 遥控1参数
const uint16_t adc_range[5] = {4095, 3780, 3970, 3640, 4095}; // 遥控2参数
void ShowAdcData(const uint32_t* adcs, const uint32_t channal);

#ifdef USE_LVGL_DISPLAY
void InitAll(lv_disp_t *disp)
{
    lv_obj_t *scr = lv_disp_get_scr_act(disp);

    // 获取屏幕分辨率
    int32_t screen_width = lv_disp_get_hor_res(disp);
    int32_t screen_height = lv_disp_get_ver_res(disp);

    const char* title_str = "Remote";
    /* 创建一个标签 */
    title = (lv_obj_t *)lv_label_create(scr);
    /* 设置标签的文本 */
    lv_label_set_text_static(title, title_str);
    /* 设置标签顶部居中 */
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 5, 5);

    // ADC 通道数据显示 - 底部均匀排列
    int32_t adc_spacing = screen_width / 5;  // 根据屏幕宽度计算间距
    int32_t adc_y = screen_height - 20;       // 底部向上20像素
    for (uint32_t i = 0; i < 5; i++) {
        adc_chanals[i] = (lv_obj_t *)lv_label_create(scr);
        lv_label_set_text(adc_chanals[i], "0");
        lv_obj_set_pos(adc_chanals[i], i * adc_spacing + 4, adc_y);
    }

    // 电池相关参数 - 右上角
    int32_t battery_width = 40;
    int32_t battery_height = 15;
    int32_t battery_x = screen_width - battery_width - 10;  // 右边距10像素
    int32_t battery_y = 5;  // 顶部向下30像素

    // 创建电池电量显示 - 右上角
    battery_bar = (lv_obj_t *)lv_bar_create(scr);
    lv_obj_set_size(battery_bar, battery_width, battery_height);
    lv_obj_set_pos(battery_bar, battery_x, battery_y);
    lv_bar_set_range(battery_bar, 0, 100);

    // 创建电池电压标签 - 显示在电量条前面
    battery_label = (lv_obj_t *)lv_label_create(scr);
    lv_label_set_text_static(battery_label, "60");
    lv_obj_set_pos(battery_label, battery_x + 5, battery_y + 1);  // 电量条左侧

    // SelectInit();
    // Serial(UART_NUM_1, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    RockerInit();
    SetRockerCallback(ShowAdcData);
    // SoftApStaInit();
    // SetUpSta("OpenWrt_R619ac_2.4G", "67123236");
    // SetUpAp("Remote", "12345678");
    PwmCtrlInit();
}

void SetupLock(_lock_t *lvgl_mux)
{
    g_lvgl_mux = lvgl_mux;
}


void ShowAdcData(const uint32_t* adcs, const uint32_t channal)
{
    char str[32];
    _lock_acquire(g_lvgl_mux);
    for (uint32_t i = 0; i < channal; i++) {
#if 0
        // 获取各通道量程
        snprintf(str, 32, "%lu", adcs[i]);
#else
        // ADC 12位精度，转换为百分比（不显示%号）
        uint32_t percent = adcs[i] * 100 / adc_range[i];
        snprintf(str, 32, "%lu", percent);
#endif
        lv_label_set_text(adc_chanals[i], str);
    }

    // 更新电池显示 (使用第5个通道，索引为4)
    if (channal > 4) {
        float voltage = AdcToVoltage(adcs[4]);
        uint8_t percent = GetBatteryPercent(voltage);

        // 更新百分比显示
        snprintf(str, 32, "%d%%", percent);
        lv_label_set_text(battery_label, str);

        // 更新电量条
        lv_bar_set_value(battery_bar, percent, LV_ANIM_OFF);
    }
    _lock_release(g_lvgl_mux);
}
#else

void ShowAdcData(const uint32_t* adcs, const uint32_t channal)
{
    char str[32];
    for (uint32_t i = 0; i < channal; i++) {
#if 0
        // 获取各通道量程
        snprintf(str, 32, "%lu", adcs[i]);
#else
        // ADC 12位精度，转换为百分比（不显示%号）
        uint32_t percent = adcs[i] * 100 / adc_range[i];
        snprintf(str, 32, "%lu", percent);
#endif
    }

    // 更新电池显示 (使用第5个通道，索引为4)
    if (channal > 4) {
        float voltage = AdcToVoltage(adcs[4]);
        uint8_t percent = GetBatteryPercent(voltage);

        // 更新百分比显示
        snprintf(str, 32, "%d%%", percent);
    }
    printf("chanal [%ld %ld %ld %ld %ld] Battery[%s]\n", adcs[0], adcs[1], adcs[2], adcs[3], adcs[4], str);
}

void InitAll(void)
{
    SelectInit();
    Serial(UART_NUM_1, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    RockerInit();
    SetRockerCallback(ShowAdcData);
    SoftApStaInit();
    // SetUpSta("OpenWrt_R619ac_2.4G", "67123236");
    SetUpAp("Remote", "12345678");
    // PwmCtrlInit();
}

#endif