//headers
#include <string.h>
#include "FECencoder.h"


//some parameters definitions
#define	SYNC_POLY		0x48	// Sync vector polynomial
#define	SCRAMBLER_POLY	0x95	// Scrambler polynomial
#define	CPOLYA			0x4F	// First convolutional encoder polynomial
#define	CPOLYB			0x6D	// Second convolutional encoder polynomial
#define	GF_POLY			0x187	// GF(256) field generator polynomial
#define	A0				255		// log(0) element of GF(256) in index form


//READ ONLY TABLES
//  The log/antilog lookup tables used for Galois field multiplication
//  in the Reed-Solomon encoder.
static unsigned char Index_of[256];
static unsigned char Alpha_to[256];
//  The code generator polynomial coefficients in index (logarithmic) form
//  for the CCSDS standard (255,223) RS code
//  Only coefficients C1-C16 are given since the polynomial is palindromic and C0 and 32
//  are 0
static unsigned char RS_poly[] = { 249, 59, 66,  4, 43, 126, 251, 97, 30,  3, 213, 50, 66, 170,  5, 24 };


//INTERNAL STATE
//  The two Reed-Solomon encoder shift registers
unsigned char RS_block[2][32];
//  How many bytes have already been given to encode_data() for this frame
static int Nbytes;
//  Interleaver write pointers
static unsigned char Bmask;
static int Bindex;
//  Scrambler shift register state
static unsigned char Scrambler;
// Convolutional encoder shift register state
static unsigned char Conv_sr;


// This is the encoder's output buffer. It is ready for transmission after
// the last call to encode_parity().
// It should be sent in order, the most significant bit of Interleaver[0]
// first.
unsigned char Interleaver[650];


// Private function prototypes
void reset_encoder(void);
void encode_byte(unsigned char c);
void encode_parity(void);
static inline int mod255(int x);
static void scramble_and_encode(unsigned char c);
static void encode_and_interleave(unsigned char c,int cnt);
static void interleave_symbol(int c);
static inline int parity(int x);


// This function initializes the encoder. It only has to be called once
// at program start
void InitFEC(void){
	int i,sr;

	// Build Galois field log/antilog tables
	Index_of[0] = A0; // log(zero) = -inf
	Alpha_to[A0] = 0; // alpha**-inf = 0
	sr = 1;
	for(i=0;i<255;i++){
		Index_of[sr] = i;
		Alpha_to[i] = sr;
		sr <<= 1;
		if(sr & 256)
		sr ^= GF_POLY;
		sr &= 255;
	}

	// Clear interleaver
	for(i=0;i<650;i++)
	Interleaver[i] = 0;

	// Generate sync vector here, place into Interleaver
	sr = 0x7f;
	for(i=0;i<65;i++){
		if(sr & 64)
		Interleaver[10*i] |= 0x80; // Every 80th bit is a sync bit
		sr = (sr << 1) | parity(sr & SYNC_POLY);
	}
	reset_encoder();
}


/* 
	Encode some data packet
	dataPtr = input data to be FECed
	z_count = returns the size of the FECed data.
	return = pointer to the output buffer containing the FECed data.
*/
uint8_t * EncodeFEC( unsigned char *dataPtr, uint16_t * z_count)
{
	int counter;
	
	reset_encoder();
	for( counter=0; counter<256; counter++ )
		encode_byte( dataPtr[counter] );
	for( counter=0; counter<64; counter++ )
		encode_parity();
	//memcpy( encodedPtr, Interleaver, sizeof(Interleaver) );
	
	*z_count = (sizeof(Interleaver)/sizeof(Interleaver[0]));
	return Interleaver;
}


// This function is called before each frame to reset the encoder for a new frame
void reset_encoder(void){
	int i,j;

	Nbytes = 0;
	Conv_sr = 0;
	Scrambler = 0xff;
	Bmask = 0x40;
	Bindex = 0;

	for(j=0;j<2;j++)
	for(i=0;i<32;i++)
	RS_block[j][i] = 0;
}


// This function is called with each user data byte to be encoded into the
// current frame. It should be called in sequence 256 times per frame, followed
// by 64 calls to encode_parity(). The Interleaver array is then ready for transmission.
void encode_byte(unsigned char c){
	unsigned char *rp;
	int i;
	unsigned char feedback;

	// Update the appropriate Reed-Solomon codeword
	rp = RS_block[Nbytes & 1];

	// Compute feedback term
	feedback = Index_of[c ^ rp[0]];

	// If feedback is non-zero, multiply by each generator polynomial coefficient and
	// add to corresponding shift register elements
	if(feedback != A0){
		int j;

		// This loop exploits the palindromic nature of the generator polynomial
		// to halve the number of discrete multiplications
		for(j=0;j<15;j++){
			unsigned char t;

			t = Alpha_to[mod255(feedback + RS_poly[j])];
			rp[j+1] ^= t; rp[31-j] ^= t;
		}
		rp[16] ^= Alpha_to[mod255(feedback + RS_poly[15])];
	}

	// Shift register one position down
	for(i=0;i<31;i++)
	rp[i] = rp[i+1];

	// Handle highest order coefficient, which is unity
	if(feedback != A0){
		rp[31] = Alpha_to[feedback];
		} else {
		rp[31] = 0;
	}
	scramble_and_encode(c);
	Nbytes++;
}


// This function should be called 64 times after the 256 data bytes
// have been passed to update_encoder. Each call scrambles, encodes and
// interleaves one byte of Reed-Solomon parity.
//
// If this routine executes quickly enough to meet real-time
// requirements, it could be incorporated into a loop at the end of
// update_encoder() that is called when Nbytes reaches 256.
void encode_parity(void){
	unsigned char c;

	c =  RS_block[Nbytes & 1][(Nbytes-256)>>1];
	scramble_and_encode(c);
	if(++Nbytes == 320){
		// Tail off the convolutional encoder
		encode_and_interleave(0,6);
	}
}


// Reduce argument modulo 255 without a divide
// In this file, x is never greater than 508 so the while could be an if
static inline int mod255(int x){
  while (x >= 255)
    x -= 255;

  return x;
}


// Internal function to scramble a byte, convolutionally encode and block interleave
static void scramble_and_encode(unsigned char c){
	int i;

	// Scramble byte
	c ^= Scrambler;
	// Update scrambler
	// It might be more efficient to use the Galois form of the PN generator
	// instead of the Fibonacci form if the parity function is not fast.
	// If RAM is available, a simple 320-byte lookup table indexed by Nbytes
	// could be used instead.
	for(i=0;i<8;i++)
	Scrambler = (Scrambler << 1) | parity(Scrambler & SCRAMBLER_POLY);
	encode_and_interleave(c,8);
}


// Internal function to convolutionally encode and block-interleave one byte (no scrambling)
static void encode_and_interleave(unsigned char c,int cnt){

	while(cnt-- != 0){
		Conv_sr = (Conv_sr << 1) | (c >> 7);
		c <<= 1;
		interleave_symbol(parity(Conv_sr & CPOLYA));
		interleave_symbol(!parity(Conv_sr & CPOLYB)); // Second encoder symbol is inverted
	}
}


// Internal function to write one binary channel symbol into the block interleaver
// and update the pointers
static void interleave_symbol(int c){
	if(c)
	Interleaver[Bindex] |= Bmask;
	else
	Interleaver[Bindex] &= ~Bmask;

	Bindex += 10; // Move forward 80 bits
	if(Bindex >= 650){
		// Past end of interleaver, step to the next row
		Bindex -= 650;
		Bmask >>= 1;
		if(Bmask == 0){
			// End of this byte, go to the next
			Bmask = 0x80;
			Bindex++;
		}
	}
}


// Return 1 if x has odd parity, 0 if even
// This function is used rather heavily, so it is worth some effort to optimize
// There may be more efficient machine-specific ways to do this, or it could
// be implemented with a lookup table
static inline int parity(int x){
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return x & 1;
}

