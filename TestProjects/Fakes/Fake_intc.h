#ifndef _FAKE_INTC_H
#define _FAKE_INTC_H

#include "intc.h"
#include "fff.h"
typedef void ((*voidFunc)(void));

DECLARE_FAKE_VOID_FUNC3(INTC_register_interrupt, voidFunc, uint32_t, uint32_t);

#endif