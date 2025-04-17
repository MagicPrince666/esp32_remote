#include "ws2812.h"
#include "driver/gpio.h"
#include "esp32/rom/ets_sys.h"

#define PIN 26 // GPIO引脚，连接到WS2812灯带的信号线
#define NUM_LEDS 30 // 灯带上的LED数量

// 定义WS2812信号的高低电平时间
#define T0H 350 // 0的高电平时间
#define T0L 600 // 0的低电平时间
#define T1H 650 // 1的高电平时间
#define T1L 300 // 1的低电平时间

Ws2812::Ws2812() {}

Ws2812::~Ws2812() {}

void Ws2812::init() {
    // Initialize GPIOs
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = 1ULL << PIN;
    io_conf.mode       = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = true;

    gpio_config(&io_conf);
}

void Ws2812::delay_ns(uint32_t ns) {
    // ESP32 CPU 时钟频率为 80MHz 或 240MHz，计算对应的周期数
    uint32_t cycles = ns * (240 / 1000); // 假设 80MHz
    uint32_t start = xthal_get_ccount();
    while (xthal_get_ccount() - start < cycles);
}

// 发送单个LED的颜色
void Ws2812::sendColor(uint32_t color) {
    for (int i = 0; i < 24; i++) { // 每个LED的颜色由24位数据组成
        uint8_t bit = (color >> (23 - i)) & 0x01; // 提取当前位
        if (bit) {
            gpio_set_level(PIN, 1);
            delay_ns(T1H); // 1的高电平时间
            gpio_set_level(PIN, 0);
            delay_ns(T1L); // 1的低电平时间
        } else {
            gpio_set_level(PIN, 1);
            delay_ns(T0H); // 0的高电平时间
            gpio_set_level(PIN, 0);
            delay_ns(T0L); // 0的低电平时间
        }
    }
}
