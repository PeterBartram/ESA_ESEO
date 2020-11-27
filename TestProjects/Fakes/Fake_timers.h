#ifndef _FAKE_TIMERS_H
#define _FAKE_TIMERS_H

#include "timers.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC5(xTimerHandle, TimerCreate, const char *, uint32_t, uint32_t, void *, tmrTIMER_CALLBACK);
DECLARE_FAKE_VALUE_FUNC2(uint32_t, TimerStart, xTimerHandle, uint32_t);
DECLARE_FAKE_VALUE_FUNC2(uint32_t, TimerStop, xTimerHandle, uint32_t);
DECLARE_FAKE_VALUE_FUNC2(uint32_t, TimerDelete, xTimerHandle, uint32_t);
DECLARE_FAKE_VALUE_FUNC2(uint32_t, TimerReset, xTimerHandle, uint32_t);
DECLARE_FAKE_VALUE_FUNC1(BaseType_t, xTimerIsTimerActive, xTimerHandle);

#endif