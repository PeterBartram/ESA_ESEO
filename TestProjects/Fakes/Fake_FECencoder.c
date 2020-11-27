#include "Fake_FECencoder.h"

DEFINE_FAKE_VOID_FUNC0(InitFEC);
DEFINE_FAKE_VALUE_FUNC2(uint8_t *, EncodeFEC, unsigned char *, uint16_t *);
