#ifndef BITSTREAM_H_
#define BITSTREAM_H_

#include <stdint.h>

typedef struct _BITSTREAM_CONTROL
{
	uint8_t byteInput;
	uint8_t bitInput;
}BITSTREAM_CONTROL;


void AddToBitStream(BITSTREAM_CONTROL * z_control, uint8_t * z_buffer, uint8_t z_bits, uint32_t z_data);

#endif /* BITSTREAM_H_ */