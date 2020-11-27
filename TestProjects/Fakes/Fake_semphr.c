#include "Fake_semphr.h"

DEFINE_FAKE_VALUE_FUNC1(SemaphoreHandle_t, xSemaphoreCreateBinaryStatic, StaticSemaphore_t);
DEFINE_FAKE_VALUE_FUNC1(uint32_t, xSemaphoreGive, SemaphoreHandle_t);
DEFINE_FAKE_VALUE_FUNC2(uint32_t, xSemaphoreTake, SemaphoreHandle_t, uint32_t);

DEFINE_FAKE_VALUE_FUNC1(SemaphoreHandle_t, xSemaphoreCreateRecursiveMutexStatic, StaticSemaphore_t);
DEFINE_FAKE_VALUE_FUNC1(uint32_t, xSemaphoreGiveRecursive, SemaphoreHandle_t);
DEFINE_FAKE_VALUE_FUNC2(uint32_t, xSemaphoreTakeRecursive, SemaphoreHandle_t, uint32_t);
