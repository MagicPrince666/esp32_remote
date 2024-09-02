#ifndef __PERIPHERAILS_H__
#define __PERIPHERAILS_H__

#include <stdint.h>
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

void InitAll(lv_disp_t *disp);

void SetupLock(SemaphoreHandle_t lvgl_mux);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
