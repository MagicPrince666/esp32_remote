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

#define ADC_MAX_VALUE    4095    // 12bit ADC
#define ADC_CHANNEL_NUM   5       // ADC通道数量

typedef void (*rocker_callback_t)(const uint32_t*, const uint32_t);

void RockerInit();

uint32_t *GetAdcData();

void SetRockerCallback(rocker_callback_t handler);

// ADC中位值校准相关函数
void CalibrateAdcCenter();
uint32_t *GetAdcCenterValues();
uint32_t GetAdcCenterValue(uint8_t channel);
void SetAdcCenterValue(uint8_t channel, uint32_t value);

#endif
