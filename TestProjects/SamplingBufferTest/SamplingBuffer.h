
#include <stdint.h>

void InitialiseSamplingBuffer(void);
uint32_t SizeSamplingBuff(void);
void	UplinkInputBufferSet(uint16_t z_data);
uint32_t UplinkInputBufferGet(float * z_data);