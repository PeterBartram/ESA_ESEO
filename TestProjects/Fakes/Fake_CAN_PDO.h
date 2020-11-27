#ifndef _FAKE_CAN_PDO_H
#define _FAKE_CAN_PDO_H

#include "CAN_PDO.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC3(CAN_OPEN_ERROR, ReceivedPayloadDataTransferProtocolRequest, uint8_t, uint32_t, uint8_t);

#endif