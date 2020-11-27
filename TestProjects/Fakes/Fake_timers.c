#include "Fake_timers.h"

DEFINE_FAKE_VALUE_FUNC5(xTimerHandle, TimerCreate, const char *, uint32_t, uint32_t, void *, tmrTIMER_CALLBACK);
DEFINE_FAKE_VALUE_FUNC2(uint32_t, TimerStart, xTimerHandle, uint32_t);
DEFINE_FAKE_VALUE_FUNC2(uint32_t, TimerStop, xTimerHandle, uint32_t);
DEFINE_FAKE_VALUE_FUNC2(uint32_t, TimerDelete, xTimerHandle, uint32_t);
DEFINE_FAKE_VALUE_FUNC2(uint32_t, TimerReset, xTimerHandle, uint32_t);
DEFINE_FAKE_VALUE_FUNC1(BaseType_t, xTimerIsTimerActive, xTimerHandle);
