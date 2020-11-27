// headers
#include <asf.h>
#include <string.h>
#include "AX25encoder.h"


static uint32_t AX25bitStuffling, AX25bytePosition, AX25bitSize;
static uint8_t AX25bitMask;
static uint8_t *AX25bitStream;


void AX25AddBit( int bitToAdd, int doStuffling );
void AX25AddByte( uint8_t byteToAdd, int doStuffling );


void AX25encode( uint8_t *outData, uint32_t *outSizeBits, uint8_t *inData, uint32_t inSize, char *dstAddr, char *srcAddr )
{
	uint32_t aux0;
	
	//reset control variables
	AX25bitStuffling = 0;
	AX25bitMask = 0x80;
	AX25bitSize = 0;
	AX25bytePosition = 0;
	AX25bitStream = outData;
	
	// add START FLAG without bit stuffling
	AX25AddByte( 0x7E, 0 );
	
	// destination addres
	for( aux0=0; aux0<7; aux0++ )
		AX25AddByte( dstAddr[aux0], 1 );
	// source address
	for( aux0=0; aux0<7; aux0++ )
		AX25AddByte( srcAddr[aux0], 1 );
	// control field
	AX25AddByte( 0x03, 1 ); // unnumbered information
	// protocol identifier field
	AX25AddByte( 0xF0, 1 ); // no protocol on layer 3
	// data
	for( aux0=0; aux0<inSize; aux0++ )
		AX25AddByte( inData[aux0], 1 );
	// crc (not implemented)
	AX25AddByte( 0x55, 1 );
	AX25AddByte( 0x55, 1 );
	
	// add END FLAG without bit stuffling
	AX25AddByte( 0x7E, 0 );
	
	// return bit size
	*outSizeBits = AX25bitSize;
	return;
}


void AX25AddBit( int bitToAdd, int doStuffling )
{
	// byte position should point to a byte with at least 1 bit space
	// bit mask should mask the next bit to be added (7 to 0)
	// DAC out buffer send MSB first, so add bit to MSB of the bit stream
	
	if( bitToAdd )
	{
		AX25bitStream[AX25bytePosition] |= AX25bitMask;
		AX25bitStuffling++;
	}
	else
	{
		AX25bitStream[AX25bytePosition] &= (~AX25bitMask);
		AX25bitStuffling = 0;
	}
	
	AX25bitSize++;
	AX25bitMask >>= 1;
	if( !AX25bitMask )
	{
		AX25bitMask = 0x80;
		AX25bytePosition++;
		AX25bitStream[AX25bytePosition] = 0;
	}
	
	if( doStuffling )
	{
		if( AX25bitStuffling >= 5 )
		{
			AX25bitStream[AX25bytePosition] &= (~AX25bitMask);
			AX25bitStuffling = 0;
		
			AX25bitSize++;
			AX25bitMask >>= 1;
			if( !AX25bitMask )
			{
				AX25bitMask = 0x80;
				AX25bytePosition++;
				AX25bitStream[AX25bytePosition] = 0;
			}
		}
	}
	
	return;
}


void AX25AddByte( uint8_t byteToAdd, int doStuffling )
{
	// receiver (AX25) expect to receiver LSB of data first
	// add LSB to MSB then
	uint32_t aux;
	
	for( aux=0; aux<8; aux++ )
	{
		AX25AddBit( !!(byteToAdd & 0x01), doStuffling );
		byteToAdd >>= 1;
	}
	
	return;
}

