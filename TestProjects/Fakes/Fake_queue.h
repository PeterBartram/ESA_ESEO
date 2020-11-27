#ifndef _FAKE_CAN_PDO_H
#define _FAKE_CAN_PDO_H

#include "queue.h"
#include "fff.h"
 
DECLARE_FAKE_VALUE_FUNC4(QueueHandle_t, xQueueCreateStatic, uint32_t, uint32_t, uint8_t *, StaticQueue_t*);
DECLARE_FAKE_VALUE_FUNC1(uint32_t, uxQueueMessagesWaiting, QueueHandle_t);
DECLARE_FAKE_VALUE_FUNC3(uint32_t, xQueueReceive, QueueHandle_t, uint8_t *, uint32_t);
DECLARE_FAKE_VALUE_FUNC3(uint32_t, xQueueSend, QueueHandle_t, const void *, uint32_t);

#endif