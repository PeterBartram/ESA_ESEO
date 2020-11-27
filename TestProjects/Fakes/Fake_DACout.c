#include "Fake_DACout.h"

DEFINE_FAKE_VOID_FUNC0(InitialiseDAC);
DEFINE_FAKE_VALUE_FUNC0(uint32_t, DACoutBuffSize);
DEFINE_FAKE_VOID_FUNC2(DACOutputBufferReady, uint8_t *, uint16_t);
DEFINE_FAKE_VALUE_FUNC0(uint8_t, TransmitReady);
DEFINE_FAKE_VOID_FUNC1(ChangeMode, TRANSMISSION_MODE);