#include "battery.h"

// 将 ADC 值转换为电池电压
float AdcToVoltage(uint32_t adc_value) {
    float adc_voltage = (adc_value / (float)ADC_MAX_VALUE) * ADC_VREF;
    return adc_voltage * VOLTAGE_RATIO;
}

// 获取电池电量百分比
uint8_t GetBatteryPercent(float voltage) {
    if (voltage >= BATTERY_MAX) return 100;
    if (voltage <= BATTERY_MIN) return 0;
    return (uint8_t)((voltage - BATTERY_MIN) / (BATTERY_MAX - BATTERY_MIN) * 100);
}
