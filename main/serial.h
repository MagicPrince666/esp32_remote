#ifndef __SERIAL_H__
#define __SERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif

class Serial
{
private:
public:
    Serial();
    ~Serial();
};

void rx_task(void *arg);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
