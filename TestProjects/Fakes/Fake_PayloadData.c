
#include "Fake_PayloadData.h"

DEFINE_FAKE_VALUE_FUNC3(PAYLOAD_DATA_ERROR, GetPayloadData, uint8_t *, uint16_t, uint16_t *);
DEFINE_FAKE_VALUE_FUNC0(uint8_t, PayloadDataPresent);