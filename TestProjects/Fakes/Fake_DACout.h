#ifndef _FAKE_DAC_H
#define _FAKE_DAC_H

#include "DACout.h"
#include "fff.h"
 

DECLARE_FAKE_VOID_FUNC0(InitialiseDAC);
DECLARE_FAKE_VALUE_FUNC0(uint32_t, DACoutBuffSize);
DECLARE_FAKE_VOID_FUNC2(DACOutputBufferReady, uint8_t *, uint16_t);
DECLARE_FAKE_VALUE_FUNC0(uint8_t, TransmitReady);
DECLARE_FAKE_VOID_FUNC1(ChangeMode, TRANSMISSION_MODE);


#endif