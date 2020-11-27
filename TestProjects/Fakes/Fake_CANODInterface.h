#ifndef _FAKE_CAN_OD_INTERFACE_H
#define _FAKE_CAN_OD_INTERFACE_H

#include "CANODInterface.h"
#include "fff.h"
#include "queue.h"

DECLARE_FAKE_VALUE_FUNC0(QueueHandle_t, GetCommandQueueHandle);

#endif