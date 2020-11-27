
#include <stdint.h>

typedef uint32_t StaticSemaphore_t;
typedef uint32_t SemaphoreHandle_t;

SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t *pxSemaphoreBuffer);
uint32_t xSemaphoreGive(SemaphoreHandle_t z_handle);
uint32_t xSemaphoreTake(SemaphoreHandle_t z_handle, uint32_t z_ticks);

SemaphoreHandle_t xSemaphoreCreateRecursiveMutexStatic(StaticSemaphore_t *pxSemaphoreBuffer);
uint32_t xSemaphoreGiveRecursive(SemaphoreHandle_t z_handle);
uint32_t xSemaphoreTakeRecursive(SemaphoreHandle_t z_handle, uint32_t z_ticks);