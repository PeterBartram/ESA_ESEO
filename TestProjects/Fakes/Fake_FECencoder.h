#ifndef _FAKE_FEC_H
#define _FAKE_FEC_H

#include "FECencoder.h"
#include "fff.h"
 
DECLARE_FAKE_VOID_FUNC0(InitFEC);
DECLARE_FAKE_VALUE_FUNC2(uint8_t *, EncodeFEC, unsigned char *, uint16_t *);

#endif