
#ifndef PAYLOAD_DATA_H_FAKE

#include "PayloadData.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC3(PAYLOAD_DATA_ERROR, GetPayloadData, uint8_t *, uint16_t, uint16_t *);
DECLARE_FAKE_VALUE_FUNC0(uint8_t, PayloadDataPresent);

#endif