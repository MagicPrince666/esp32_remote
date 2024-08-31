#ifndef __PWM_CTRL_H__
#define __PWM_CTRL_H__

#include "driver/mcpwm_prelude.h"
#include "driver/gpio.h"

class PwmCtrl
{
public:
    PwmCtrl();
    ~PwmCtrl();

    void SetAngle(int angle);

private:
    mcpwm_cmpr_handle_t comparator_;
};

#endif
