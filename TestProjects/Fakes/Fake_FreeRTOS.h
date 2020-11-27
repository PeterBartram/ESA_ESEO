#ifndef _FAKE_FREE_RTOS_H
#define _FAKE_FREE_RTOS_H

#include "FreeRTOS.h"
#include "fff.h"

DECLARE_FAKE_VOID_FUNC1(vTaskDelay, uint32_t);

#endif