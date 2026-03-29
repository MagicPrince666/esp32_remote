#include "business.h"
#include "rocker.h"
#include "pwm_ctrl.h"
#include "softap_sta.h"
#include "battery.h"
#include "select.h"
#include "serial.h"
#include "esp_log.h"
#include <sys/param.h>
#include <string.h>

#ifdef USE_LVGL_DISPLAY
static _lock_t *g_lvgl_mux = NULL;
static lv_obj_t * title;
static lv_obj_t * adc_chanals[5];
static lv_obj_t * battery_label;  // 电池电压标签
static lv_obj_t * battery_bar;    // 电池电量条
#endif
const uint16_t adc_range[5] = {4095, 4095, 3821, 3740, 4095}; // 遥控1参数
// const uint16_t adc_range[5] = {4095, 3780, 3970, 3640, 4095}; // 遥控2参数
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
    
    // 等待一段时间让ADC稳定后进行中位值校准
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    CalibrateAdcCenter();
    
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
    int32_t percent[5] = {0};
    _lock_acquire(g_lvgl_mux);
    for (uint32_t i = 0; i < channal; i++) {
#if 0
        // 获取各通道量程
        snprintf(str, 32, "%lu", adcs[i]);
#else
        if (i == 4) {
            percent[i] = adcs[i] * 100 / adc_range[i];
        } else {
            // 获取当前通道的中位值
            uint32_t center = GetAdcCenterValue(i);
            
            // 计算相对于中位值的偏移量
            int32_t offset = (int32_t)adcs[i] - (int32_t)center;
            
            // 计算线性化后的值，中心位置为50%
            // 使用量程的一半作为最大偏移量
            uint32_t max_offset = adc_range[i] / 2;
            
            // 将偏移量转换为百分比，中心位置为50%
            // 超过中位值的按50-100显示，增量方向从小到大
            // 低于中位值的按0-49显示，增长方向为从大到小
            if (offset >= 0) {
                // 超过中位值，按50-100显示
                percent[i] = 50 + (offset * 50) / max_offset;
            } else {
                // 低于中位值，按0-49显示
                // 使用绝对值计算偏移量，然后从50减去
                percent[i] = 50 - ((-offset) * 50) / center;
            }
            
            // 限制百分比范围在0-100之间
            if (percent[i] < 0) percent[i] = 0;
            if (percent[i] > 100) percent[i] = 100;
        }
        
        int len = snprintf(str, 32, "%3ld", percent[i]);
        str[len] = 0;
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

struct RemoteState {
    bool lose_signal;   // 失控标识

    bool front; // 前进按钮
    bool back; // 后退按钮
    bool left; // 左转按钮
    bool right; // 右转按钮
    bool select; // 选择按钮
    bool start; // 开始按钮
    bool l1; // 左顶部按钮1
    bool l2; // 左顶部按钮2
    bool r1; // 右顶部按钮1
    bool r2; // 右顶部按钮2
    bool adl; // 左摇杆按钮
    bool adr; // 右摇杆按钮
    bool triangle; // 三角按钮
    bool quadrilateral; // 四边形按钮
    bool rotundity; // 园形按钮
    bool fork; // 叉按钮
    float adslx;   // 左摇杆x轴
    float adsly;   // 左摇杆y轴
    float adsrx;   // 右摇杆x轴
    float adsry;   // 右摇杆y轴
    float ads[12];   // 扩展通道 sbus 16路通道都是模拟量
};

static const char *TAG = "peripherals";
static int sock = -1;  // UDP socket
static struct sockaddr_in dest_addr;
static bool udp_initialized = false;

// UDP初始化函数
static int udp_init(const char *ip, uint16_t port)
{
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }

    dest_addr.sin_addr.s_addr = inet_addr(ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    ESP_LOGI(TAG, "UDP initialized, target: %s:%d", ip, port);
    return s;
}

// 发送RemoteState数据
static void send_remote_state(const struct RemoteState *state)
{
    if (!udp_initialized || sock < 0) {
        return;
    }

    int err = sendto(sock, state, sizeof(struct RemoteState), 0, 
                    (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        // ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
    }
}

void ShowAdcData(const uint32_t* adcs, const uint32_t channal)
{
    char str[32];
    int32_t percent[5] = {0};
    for (uint32_t i = 0; i < channal; i++) {
#if 0
        // 获取各通道量程
        snprintf(str, 32, "%lu", adcs[i]);
#else
        if (i == 4) {
            percent[i] = adcs[i] * 100 / adc_range[i];
        } else {
            // 获取当前通道的中位值
            uint32_t center = GetAdcCenterValue(i);
            
            // 计算相对于中位值的偏移量
            int32_t offset = (int32_t)adcs[i] - (int32_t)center;
            
            // 将偏移量转换为百分比，中心位置为50%
            // 超过中位值的按50-100显示，增量方向从小到大
            // 低于中位值的按0-49显示，增长方向为从大到小
            if (offset >= 0) {
                // 计算线性化后的值，中心位置为50%
                // 使用量程的一半作为最大偏移量
                uint32_t max_offset = adc_range[i] / 2;
                // 超过中位值，按50-100显示
                percent[i] = 50 + (offset * 50) / max_offset;
            } else {
                // 低于中位值，按0-49显示
                // 使用绝对值计算偏移量，然后从50减去
                percent[i] = 50 - ((-offset) * 50) / center;
            }
            
            // 限制百分比范围在0-100之间
            if (percent[i] < 0) percent[i] = 0;
            if (percent[i] > 100) percent[i] = 100;
        }
        
        int len = snprintf(str, 32, "%3ld", percent[i]);
        str[len] = 0;
#endif
        ShowString(10 + i*44, 300, strlen(str) * 8, 16, 16, str);
    }

    // 更新电池显示 (使用第5个通道，索引为4)
    if (channal > 4) {
        float voltage = AdcToVoltage(adcs[4]);
        uint8_t percent = GetBatteryPercent(voltage);

        // 更新百分比显示
        int len = snprintf(str, 32, "%3d%%", percent);
        str[len] = 0;
        ShowString(200, 10, strlen(str) * 8, 16, 16, str);
    }
    
    // 创建RemoteState结构体并发送
    struct RemoteState state = {0};
    
    // 将摇杆数据转换为0-1的浮点数
    // 左摇杆X轴 (adc_raw_[0])
    state.adslx = (float)percent[0] / 100.0f;
    if (state.adslx < 0.0f) state.adslx = 0.0f;
    if (state.adslx > 1.0f) state.adslx = 1.0f;
    // 左摇杆Y轴 (adc_raw_[1])
    state.adsly = (float)percent[1] / 100.0f;
    if (state.adsly < 0.0f) state.adsly = 0.0f;
    if (state.adsly > 1.0f) state.adsly = 1.0f;
    // 右摇杆X轴 (adc_raw_[2])
    state.adsrx = (float)percent[2] / 100.0f;
    if (state.adsrx < 0.0f) state.adsrx = 0.0f;
    if (state.adsrx > 1.0f) state.adsrx = 1.0f;
    state.adsrx = 1.0f - state.adsrx; // 反转X轴方向
    // 右摇杆Y轴 (adc_raw_[3])
    state.adsry = (float)percent[3] / 100.0f;
    if (state.adsry < 0.0f) state.adsry = 0.0f;
    if (state.adsry > 1.0f) state.adsry = 1.0f;
    state.adsry = 1.0f - state.adsry; // 反转Y轴方向
    
    // 发送RemoteState到对端
    send_remote_state(&state);
}

void ShowIp(ip_event_ap_staipassigned_t* event, bool connect)
{
    char str[64];
    int len = snprintf(str, sizeof(str), MACSTR, MAC2STR(event->mac));
    str[len] = 0;
    ShowString(48, 90, strlen(str) * 8, 16, 16, str);
    len = snprintf(str, sizeof(str), IPSTR, IP2STR(&event->ip));
    str[len] = 0;
    ShowString(48, 110, strlen(str) * 8, 16, 16, str);

    if (connect) {
        if (sock > 0) {
            return;
        }
        sock = udp_init("192.168.34.168", 5555);
        if (sock >= 0) {
            udp_initialized = true;
        }
    } else {
        if (sock >= 0) {
            close(sock);
            udp_initialized = false;
        }
    }
}

void ShowIpAndConnect(ip_event_ap_staipassigned_t* event, bool connect)
{
    char str[64];
    int len = snprintf(str, sizeof(str), MACSTR, MAC2STR(event->mac));
    str[len] = 0;
    ShowString(48, 50, strlen(str) * 8, 16, 16, str);
    len = snprintf(str, sizeof(str), IPSTR, IP2STR(&event->ip));
    str[len] = 0;
    ShowString(48, 70, strlen(str) * 8, 16, 16, str);
    if (connect) {
        // 初始化UDP连接到对端的5555端口
        if (sock > 0) {
            return;
        }
        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip));
        sock = udp_init(ip_str, 5555);
        if (sock >= 0) {
            udp_initialized = true;
        }
    } else {
        if (sock >= 0) {
            close(sock);
            udp_initialized = false;
        }
    }
}

void InitAll(void)
{
    SelectInit();
    Serial(UART_NUM_1, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    RockerInit();
    SetRockerCallback(ShowAdcData);
    
    // 等待一段时间让ADC稳定后进行中位值校准
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    CalibrateAdcCenter();
    
    SoftApStaInit();
    SetIpCallback(ShowIpAndConnect);
    SetStaIpCallback(ShowIp);
    SetUpSta("OpenWrt_R619ac_2.4G", "67123236");
    // SetUpAp("Remote", "12345678");
    PwmCtrlInit();
}

#endif