#include <stdint.h>

typedef uint32_t QueueHandle_t;
typedef uint32_t StaticQueue_t;

 QueueHandle_t xQueueCreateStatic(uint32_t uxQueueLength, uint32_t uxItemSize, uint8_t *pucQueueStorageBuffer, StaticQueue_t *pxQueueBuffer );
 uint32_t uxQueueMessagesWaiting(QueueHandle_t z_handle);
 uint32_t xQueueReceive(QueueHandle_t z_handle, uint8_t * z_buffer, uint32_t z_ticks);
 uint32_t xQueueSend(QueueHandle_t, const void *, uint32_t);