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

typedef void (*rocker_callback_t)(const uint32_t*, const uint32_t);

void Rocker();

uint32_t *GetAdcData();

#endif
