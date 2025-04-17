#ifndef __WS2812_H__
#define __WS2812_H__

#include <stdint.h>

class Ws2812 {
public:
    Ws2812();
    ~Ws2812();

    void init();

    void sendColor(uint32_t color);

private:
    void delay_ns(uint32_t ns);
};

#endif
