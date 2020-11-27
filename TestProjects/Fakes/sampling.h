#ifndef _FAKE_TIMERS_H
#define _FAKE_TIMERS_H

#include <stdint.h>

void InitSampling(void);
uint32_t SizeSamplingBuff(void);
uint32_t GetSamplingBuffer( int16_t *ptr, uint32_t size );
uint8_t GetSampleFromBuffer(float * z_value);


#endif