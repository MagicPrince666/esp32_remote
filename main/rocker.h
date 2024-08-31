/**
 * @file rocker.h
 * @author 黄李全 (846863428@qq.com)
 * @brief 摇杆检测
 * @version 0.1
 * @date 2024-04-04
 * @copyright 个人版权所有 Copyright (c) 2023
 */
#ifndef __ROCKER_H__
#define __ROCKER_H__

#include "esp_adc/adc_continuous.h"
class Rocker
{
public:
    Rocker();
    ~Rocker();

    uint32_t *GetAdcData() {
        return adc_raw_;
    }

private:
    adc_continuous_handle_t handle_;
    uint32_t adc_raw_[8];

    static void adc_read_task(void *param);
};

#endif
