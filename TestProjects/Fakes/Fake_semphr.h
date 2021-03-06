#ifndef _FAKE_SEMPHR_H
#define _FAKE_SEMPHR_H

#include "semphr.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC1(SemaphoreHandle_t, xSemaphoreCreateBinaryStatic, StaticSemaphore_t);
DECLARE_FAKE_VALUE_FUNC1(uint32_t, xSemaphoreGive, SemaphoreHandle_t);
DECLARE_FAKE_VALUE_FUNC2(uint32_t, xSemaphoreTake, SemaphoreHandle_t, uint32_t);

DECLARE_FAKE_VALUE_FUNC1(SemaphoreHandle_t, xSemaphoreCreateRecursiveMutexStatic, StaticSemaphore_t);
DECLARE_FAKE_VALUE_FUNC1(uint32_t, xSemaphoreGiveRecursive, SemaphoreHandle_t);
DECLARE_FAKE_VALUE_FUNC2(uint32_t, xSemaphoreTakeRecursive, SemaphoreHandle_t, uint32_t);

#endif