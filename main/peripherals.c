#include "peripherals.h"
#include "serial.h"
#include "rocker.h"
#include "pwm_ctrl.h"
#include "softap_sta.h"
#include "select.h"

#define TXD1_PIN 18
#define RXD1_PIN 5

static _lock_t *g_lvgl_mux = NULL;

lv_disp_t *g_disp = NULL;
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
    lv_label_set_text_static(title, "Remote Controler!");
    /* 设置标签的位置 */
    lv_obj_set_pos(title, 72, 0);

    for (uint32_t i = 0; i < 5; i++) {
        adc_chanals[i] = (lv_obj_t *)lv_label_create(scr);
        /* 设置标签的文本 */
        lv_label_set_text(adc_chanals[i], "0");
        /* 设置标签的位置 */
        lv_obj_set_pos(adc_chanals[i], 48 * i + 4, 20);
    }
    SelectInit();
    // lv_obj_set_size(title, 100, 20);
    Serial(UART_NUM_1, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    RockerInit();
    SetRockerCallback(ShowAdcData);
    // g_wifi = new SoftApSta(); 
    // g_wifi->Init();
    // g_wifi->SetUpSta("OpenWrt_R619ac_2.4G", "67123236");
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
        int32_t lenght = snprintf(str, 128, "%ld", adcs[i]);
        str[lenght] = 0;
        lv_label_set_text(adc_chanals[i], str);
    }
    _lock_release(g_lvgl_mux);
}
