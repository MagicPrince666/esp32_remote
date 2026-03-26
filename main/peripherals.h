#ifndef __PERIPHERAILS_H__
#define __PERIPHERAILS_H__

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <unistd.h>
#include <sys/lock.h>
#include <sys/param.h>
#ifdef USE_LVGL_DISPLAY
#include "lvgl.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_LVGL_DISPLAY
void InitAll(lv_disp_t *disp);
void SetupLock(_lock_t *lvgl_mux);
#else
void InitAll(void);
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
