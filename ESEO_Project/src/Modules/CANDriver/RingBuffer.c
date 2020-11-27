#include "RingBuffer.h"
#include "string.h"

/* Control variables for our ring buffer. */
static CAN_PACKET * memory;
static uint32_t size;
static CAN_PACKET * end;
static CAN_PACKET * start;
static uint32_t count;
static CAN_PACKET * in;
static CAN_PACKET * out;

/* initialise the ring buffer based upon the memory and size passed in. */
void RingBufferInit(CAN_PACKET * z_memory, uint32_t z_size)
{
	memory = z_memory;
	size = z_size;
	start = memory;
	end = memory + size - 1;
	in = start;
	out = start;
	count = 0;
}

/* Add a single element to the ring buffer. */
uint8_t RingBufferAddElement(CAN_PACKET * z_value)
{
	uint8_t retVal = 0;
	if(z_value)
	{
		if(count < size)
		{
			in->dataSize = z_value->dataSize;
			in->id = z_value->id;
			in->requestType = z_value->requestType;
			memcpy(in->data, z_value->data, z_value->dataSize);	/* dataSize checked in calling function. */
			retVal = 1;
			count++;

			if(in == end)
			{
				in = start;
			}
			else
			{
				in++;
			}
		}
	}
	return retVal;
}

/* Remove a single element from the ring buffer. */
CAN_PACKET RingBufferGetElement(void)
{
	CAN_PACKET retVal;

	if(count > 0)
	{
		/* Copy acoss each element */
		retVal.dataSize = out->dataSize;
		retVal.id = out->id;
		retVal.requestType = out->requestType;
		memcpy(retVal.data, out->data, out->dataSize);	/*"todo ensure this string length! */

		count --;
		if(out == end)
		{
			out = start;
		}
		else
		{
			out++;
		}
	}
	return retVal;
}

/* Return the number of elements in the ring buffer. */
uint32_t RingBufferGetCount(void)
{
	return count;
}

/* Returns non-zero if the buffer is full */
uint32_t RingBufferIsFull(void)
{
	if(count <= size)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

uint32_t RingBufferIsEmpty(void)
{
	return count ? 0 : 1;
}
