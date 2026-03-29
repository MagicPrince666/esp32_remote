#include <string.h>
#include "rocker.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#define EXAMPLE_ADC_UNIT ADC_UNIT_1
#define _EXAMPLE_ADC_UNIT_STR(unit) #unit
#define EXAMPLE_ADC_UNIT_STR(unit) _EXAMPLE_ADC_UNIT_STR(unit)
#define EXAMPLE_ADC_CONV_MODE ADC_CONV_SINGLE_UNIT_1
#define EXAMPLE_ADC_ATTEN ADC_ATTEN_DB_0
#define EXAMPLE_ADC_BIT_WIDTH SOC_ADC_DIGI_MAX_BITWIDTH

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define EXAMPLE_ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define EXAMPLE_ADC_GET_CHANNEL(p_data) ((p_data)->type1.channel)
#define EXAMPLE_ADC_GET_DATA(p_data) ((p_data)->type1.data)
#else
#define EXAMPLE_ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define EXAMPLE_ADC_GET_CHANNEL(p_data) ((p_data)->type2.channel)
#define EXAMPLE_ADC_GET_DATA(p_data) ((p_data)->type2.data)
#endif

#define EXAMPLE_READ_LEN 256
static TaskHandle_t s_task_handle;
static const char *TAG = "rocker";
static adc_continuous_handle_t handle_;
static uint32_t adc_raw_[8];
rocker_callback_t reflash_function_; // 接收回调

// ADC中位值校准相关变量
static uint32_t adc_center_values[ADC_CHANNEL_NUM];  // 存储各通道的中位值
static bool adc_calibrated = false;                   // ADC是否已校准

static adc_channel_t channel[5] = {ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_6, ADC_CHANNEL_7};

static void adc_read_task(void *param);

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    // Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size    = EXAMPLE_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle_));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 20 * 1000,
        .conv_mode      = EXAMPLE_ADC_CONV_MODE,
        .format         = EXAMPLE_ADC_OUTPUT_TYPE,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num                                         = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten     = EXAMPLE_ADC_ATTEN;
        adc_pattern[i].channel   = channel[i] & 0x7;
        adc_pattern[i].unit      = EXAMPLE_ADC_UNIT;
        adc_pattern[i].bit_width = EXAMPLE_ADC_BIT_WIDTH;

        ESP_LOGI(TAG, "adc_pattern[%d].atten is :%" PRIx8, i, adc_pattern[i].atten);
        ESP_LOGI(TAG, "adc_pattern[%d].channel is :%" PRIx8, i, adc_pattern[i].channel);
        ESP_LOGI(TAG, "adc_pattern[%d].unit is :%" PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle_, &dig_cfg));

    *out_handle = handle_;
}

void adc_read_task(void *param)
{
    esp_err_t ret;
    uint32_t ret_num                 = 0;
    uint8_t result[EXAMPLE_READ_LEN] = {0};
    memset(result, 0xcc, EXAMPLE_READ_LEN);

    s_task_handle = xTaskGetCurrentTaskHandle();

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle_, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle_));

    while (1) {
        /**
         * This is to show you the way to use the ADC continuous mode driver event callback.
         * This `ulTaskNotifyTake` will block when the data processing in the task is fast.
         * However in this example, the data processing (print) is slow, so you barely block here.
         *
         * Without using this event callback (to notify this task), you can still just call
         * `adc_continuous_read()` here in a loop, with/without a certain block timeout.
         */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        char unit[] = EXAMPLE_ADC_UNIT_STR(EXAMPLE_ADC_UNIT);
        while (1) {
            ret = adc_continuous_read(handle_, result, EXAMPLE_READ_LEN, &ret_num, 0);
            if (ret == ESP_OK) {
                // ESP_LOGI("TASK", "ret is %x, ret_num is %" PRIu32 " bytes", ret, ret_num);
                for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                    adc_digi_output_data_t *p = (adc_digi_output_data_t *)&result[i];
                    uint32_t chan_num         = EXAMPLE_ADC_GET_CHANNEL(p);
                    uint32_t data             = EXAMPLE_ADC_GET_DATA(p);
                    /* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
                    if (chan_num < SOC_ADC_CHANNEL_NUM(EXAMPLE_ADC_UNIT)) {
                        // ESP_LOGI(TAG, "Unit: %s, Channel: %" PRIu32 ", Value: %" PRIx32, unit, chan_num, data);
                        if (chan_num == 0) {
                            if (adc_raw_[3] != data) {
                                adc_raw_[3] = data;
                            }
                        } else if (chan_num == 3) {
                            if (adc_raw_[2] != data) {
                                adc_raw_[2] = data;
                            }
                        } else if (chan_num == 4) {
                            if (adc_raw_[4] != data) {
                                adc_raw_[4] = data;
                            }
                        } else if (chan_num == 6) {
                            if (adc_raw_[0] != data) {
                                adc_raw_[0] = data;
                            }
                        }  else if (chan_num == 7) {
                            if (adc_raw_[1] != data) {
                                adc_raw_[1] = data;
                            }
                        }
                    } else {
                        ESP_LOGW(TAG, "Invalid data [%s_%" PRIu32 "_%" PRIx32 "]", unit, chan_num, data);
                    }
                }
                if (reflash_function_) {
                    reflash_function_(adc_raw_, 5);
                } else {
                    ESP_LOGI(TAG, "lx = %"PRIx32"\tly = %"PRIx32"\trx = %"PRIx32"\try = %"PRIx32"\tpwoer = %"PRIx32, 
                        adc_raw_[0],
                        adc_raw_[1],
                        adc_raw_[2],
                        adc_raw_[3],
                        adc_raw_[4]);
                }
                /**
                 * Because printing is slow, so every time you call `ulTaskNotifyTake`, it will immediately return.
                 * To avoid a task watchdog timeout, add a delay here. When you replace the way you process the data,
                 * usually you don't need this delay (as this task will block for a while).
                 */
                vTaskDelay(10 / portTICK_PERIOD_MS);
            } else if (ret == ESP_ERR_TIMEOUT) {
                // We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
                break;
            }
        }
    }

    ESP_ERROR_CHECK(adc_continuous_stop(handle_));
    ESP_ERROR_CHECK(adc_continuous_deinit(handle_));
}

void RockerInit() {
    handle_ = NULL;
    memset(adc_raw_, 0, sizeof(adc_raw_));
    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle_);
    xTaskCreate(adc_read_task, "adc_read_task", 4 * 1024, NULL, 5, NULL);
}

uint32_t *GetAdcData() {
    return adc_raw_;
}

void SetRockerCallback(rocker_callback_t handler)
{
    reflash_function_ = handler;
}

/**
 * @brief 校准ADC中位值
 * 
 * 该函数在系统启动时调用，通过采集一段时间内的ADC值，
 * 计算每个通道的平均值作为中位值。这样可以消除硬件差异，
 * 确保摇杆在中心位置时输出接近0的值。
 * 注意：通道5(索引为4)是电池电压检测通道，不参与校准。
 */
void CalibrateAdcCenter()
{
    const int sample_count = 100;  // 每个通道采集的样本数
    const int delay_ms = 10;       // 采样间隔(毫秒)
    uint32_t sum[ADC_CHANNEL_NUM] = {0};
    
    ESP_LOGI(TAG, "开始ADC中位值校准...");
    
    // 采集每个通道的多个样本
    for (int i = 0; i < sample_count; i++) {
        // 获取当前ADC值
        uint32_t *current_adc = GetAdcData();
        
        // 累加前4个通道的值(不包含通道5)
        for (int j = 0; j < ADC_CHANNEL_NUM - 1; j++) {
            sum[j] += current_adc[j];
        }
        
        // 延迟一段时间再采集下一个样本
        vTaskDelay(delay_ms / portTICK_PERIOD_MS);
    }
    
    // 计算前4个通道的平均值作为中位值
    for (int i = 0; i < ADC_CHANNEL_NUM - 1; i++) {
        adc_center_values[i] = sum[i] / sample_count;
        ESP_LOGI(TAG, "通道%d的中位值: %lu", i, adc_center_values[i]);
    }
    
    // 通道5(索引为4)是电池电压检测通道，不参与校准
    adc_center_values[4] = 0;
    
    adc_calibrated = true;
    ESP_LOGI(TAG, "ADC中位值校准完成");
}

/**
 * @brief 获取所有通道的中位值
 * 
 * @return uint32_t* 指向中位值数组的指针
 */
uint32_t *GetAdcCenterValues()
{
    return adc_center_values;
}

/**
 * @brief 获取指定通道的中位值
 * 
 * @param channel 通道索引
 * @return uint32_t 指定通道的中位值
 */
uint32_t GetAdcCenterValue(uint8_t channel)
{
    if (channel >= ADC_CHANNEL_NUM) {
        ESP_LOGW(TAG, "无效的通道索引: %d", channel);
        return 0;
    }
    
    return adc_center_values[channel];
}

/**
 * @brief 设置指定通道的中位值
 * 
 * @param channel 通道索引
 * @param value 要设置的中位值
 */
void SetAdcCenterValue(uint8_t channel, uint32_t value)
{
    if (channel >= ADC_CHANNEL_NUM) {
        ESP_LOGW(TAG, "无效的通道索引: %d", channel);
        return;
    }
    
    adc_center_values[channel] = value;
    adc_calibrated = true;
    ESP_LOGI(TAG, "通道%d的中位值设置为: %lu", channel, value);
}