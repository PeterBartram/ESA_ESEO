
#include <string.h>     /* strcat */
#include <stdlib.h>     /* strtol */
#include <stdint.h>
#include "BitStream.h"

static void set_bitfield(uint8_t *buf, uint16_t buflen, uint16_t off, uint8_t bits, uint32_t val);

void AddToBitStream(BITSTREAM_CONTROL * z_control, uint8_t * z_buffer, uint8_t z_bits, uint32_t z_data)
{
	uint16_t bitOffset = (z_control->byteInput * 8) + z_control->bitInput;
	set_bitfield(z_buffer, 256, bitOffset, z_bits, z_data);
	z_control->bitInput += z_bits;
	
	z_control->byteInput += z_control->bitInput / 8;
	z_control->bitInput = z_control->bitInput % 8;
}

static void set_bitfield(uint8_t *buf, uint16_t buflen, uint16_t off, uint8_t bits, uint32_t val)
{
	uint8_t msb = bits - 1;
	uint16_t idx = off / 8;
	uint8_t bit = 7 - (off % 8);
	uint32_t msk = 1 << msb;
	
	while (msk != 0 && idx < buflen)
	{
		if (val&msk)
		{
			buf[idx] |= (1 << bit);
		}
		else
		{
			buf[idx] &= ~(1 << bit);
		}
		msk >>= 1;
		if (--bit == 0xff) // Did we underflow?
		{
			bit = 7;
			++idx;
		}
	}
}
