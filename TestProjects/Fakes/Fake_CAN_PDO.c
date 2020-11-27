#include "Fake_CAN_PDO.h"

DEFINE_FAKE_VALUE_FUNC3(CAN_OPEN_ERROR, ReceivedPayloadDataTransferProtocolRequest, uint8_t, uint32_t, uint8_t);