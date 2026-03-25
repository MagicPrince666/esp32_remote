#ifndef __BATTERY_H__
#define __BATTERY_H__

#include <stdint.h>

// 电池电压检测参数
#define ADC_MAX_VALUE    4095    // 12bit ADC
#define ADC_VREF         1.1     // ADC 参考电压
#define R1               1000    // 分压电阻 R1 = 1K
#define R2               5100    // 分压电阻 R2 = 5.1K
#define VOLTAGE_RATIO    ((R1 + R2) / (float)R1)  // 分压比 = 6.1
#define BATTERY_MIN      3.7    // 最低电压
#define BATTERY_MAX      4.2    // 满电电压

float AdcToVoltage(uint32_t adc_value);
uint8_t GetBatteryPercent(float voltage);

#endif // __BATTERY_H__

