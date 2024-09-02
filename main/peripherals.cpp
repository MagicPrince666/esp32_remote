#include "peripherals.h"
#include "serial.h"
#include "rocker.h"
#include "pwm_ctrl.h"

Serial *g_serial = nullptr;
#define TXD1_PIN 18
#define RXD1_PIN 5

Rocker *g_rocker = nullptr;
PwmCtrl *g_pwm = nullptr;
lv_disp_t *g_disp = nullptr;
static lv_obj_t * title;
static lv_obj_t * adc_chanals[5];
void ShowAdcData(const uint32_t* adcs, const uint32_t channal);

void InitAll(lv_disp_t *disp)
{
    g_disp = disp;
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    /* 创建一个标签 */
    title = (lv_obj_t *)lv_label_create(scr);
    /* 设置标签的文本 */
    lv_label_set_text_static(title, "Hello, LVGL!");
    /* 设置标签的位置 */
    lv_obj_set_pos(title, 72, 0);

    for (uint32_t i = 0; i < 5; i++) {
        adc_chanals[i] = (lv_obj_t *)lv_label_create(scr);
        /* 设置标签的文本 */
        lv_label_set_text(adc_chanals[i], "0");
        /* 设置标签的位置 */
        lv_obj_set_pos(adc_chanals[i], 48 * i + 4, 20);
    }
    // lv_obj_set_size(title, 100, 20);
    g_serial = new Serial(UART_NUM_1, TXD1_PIN, RXD1_PIN);
    g_rocker = new Rocker();
    // g_pwm = new PwmCtrl();
    g_rocker->SetCallback(std::bind(&ShowAdcData, std::placeholders::_1, std::placeholders::_2));
}

void ShowAdcData(const uint32_t* adcs, const uint32_t channal)
{
    char str[128];
    for (uint32_t i = 0; i < channal; i++) {
        int32_t lenght = snprintf(str, 128, "%ld", adcs[i]);
        str[lenght] = 0;
        lv_label_set_text(adc_chanals[i], str);
    }
}