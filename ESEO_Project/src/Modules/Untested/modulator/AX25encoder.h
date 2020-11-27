#ifndef	__AX25_ENCODER__
#define	__AX25_ENCODER__


void AX25encode( uint8_t *outData, uint32_t *outSizeBits, uint8_t *inData, uint32_t inSize, char *dstAddr, char *srcAddr );


#endif	// __AX25_ENCODER__