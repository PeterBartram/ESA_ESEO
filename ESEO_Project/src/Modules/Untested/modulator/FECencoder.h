#ifndef	__FEC_ENCODER_H__
#define __FEC_ENCODER_H__

#include <stdint.h>

// public function prototypes
void InitFEC(void);
uint8_t * EncodeFEC( unsigned char *dataPtr, uint16_t * z_count);


#endif	//__FEC_ENCODER_H__