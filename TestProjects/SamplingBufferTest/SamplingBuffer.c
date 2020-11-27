#include "SamplingBuffer.h"
#include <string.h>

#define SAMPLE_BUFFER_SIZE 2048

uint16_t samplingBuffer[SAMPLE_BUFFER_SIZE];

uint32_t bufferCount;
uint32_t bufferInputIndex = 0;
uint32_t bufferOutputIndex = 0;

void InitialiseSamplingBuffer(void)
{
	memset(&samplingBuffer[0], 0, sizeof(samplingBuffer));
	bufferCount = 0;
	bufferInputIndex = 0;
	bufferOutputIndex = 0;
}

uint32_t SizeSamplingBuff(void)
{
	return bufferCount;
}

void UplinkInputBufferSet(uint16_t z_data)
{
	if(bufferCount < SAMPLE_BUFFER_SIZE)
	{
		bufferCount++;
	}

	if(bufferInputIndex >= SAMPLE_BUFFER_SIZE)
	{
		bufferInputIndex = 0;
	}

	samplingBuffer[bufferInputIndex] = z_data;
	bufferInputIndex++;
}

uint32_t UplinkInputBufferGet(float * z_data)
{
	uint32_t retVal = 0;

	if(z_data != 0 && bufferCount != 0)
	{
		if(bufferOutputIndex >= SAMPLE_BUFFER_SIZE)
		{
			bufferOutputIndex = 0;
		}
		bufferCount--;
		retVal = bufferCount;
		*z_data = samplingBuffer[bufferOutputIndex];	//((samplingBuffer[bufferOutputIndex] - 1024) / 1024); - used in final design for scaling values.
		bufferOutputIndex++;
	}
	return retVal;
}