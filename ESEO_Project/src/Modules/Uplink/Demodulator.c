/*
 * Demodulator.c
 *
 * Created: 28-Jun-16 2:46:45 PM
 *  Author: Pete
 */ 
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "Demodulator.h"
#include "Decoder.h"

#define PI 3.14159265358979323846
#define SPHASEINC (0x10000u*DEMOD_BAUD*DEMOD_SUBSAMP/DEMOD_FREQ_SAMP)	//phase incrementation each sub-sample (PS.: 0x10000 would be 2PI)
#define ADC_MAX_VALUE	2048

static DEMODULATOR_STATE demodState;		// This structure controls the AFSK demodulation.

/* Correlation demodulator signals - used to compute the integrations and guess the input bit */
static int32_t corr_mark_i[DEMOD_CORRELATION_WINDOW_SIZE];
static int32_t corr_mark_q[DEMOD_CORRELATION_WINDOW_SIZE];
static int32_t corr_space_i[DEMOD_CORRELATION_WINDOW_SIZE];
static int32_t corr_space_q[DEMOD_CORRELATION_WINDOW_SIZE];

static int32_t fullBuffer[DEMOD_BUFF_LEN];	// Used for storing data removed from the ADCs previously but not used.

static int64_t fsqr(int64_t f);
static int32_t mac(const int32_t *a, const int32_t *b, unsigned int size);

void AFSKInit(void)
{
	float f;
	int i;

	/* Clear the AFSK structure */
	memset(&demodState, 0, sizeof(demodState));
	
	/* Clear the decoding buffers. */
	memset(&fullBuffer, 0, sizeof(fullBuffer));
	
	//generate correlation demodulation signals
	//  signals with the length of a correlation window
	for (f = 0, i = 0; i < DEMOD_CORRELATION_WINDOW_SIZE; i++) {	//signals for mark frequency
		corr_mark_i[i] = (uint32_t)(ADC_MAX_VALUE * ((cos(f))/2));
		corr_mark_q[i] = (uint32_t)(ADC_MAX_VALUE * ((sin(f))/2));
		f += 2.0*PI*DEMOD_FREQ_MARK/DEMOD_FREQ_SAMP;
	}
	for (f = 0, i = 0; i < DEMOD_CORRELATION_WINDOW_SIZE; i++) {	//signals for space frequency
		corr_space_i[i] = (uint32_t)(ADC_MAX_VALUE * ((cos(f))/2));
		corr_space_q[i] = (uint32_t)(ADC_MAX_VALUE * ((sin(f))/2));
		f += 2.0*PI*DEMOD_FREQ_SPACE/DEMOD_FREQ_SAMP;
	}
	
	return;
}

__attribute__((optimize("O3")))
void AFSKDemod(int32_t * z_buffer, uint32_t length)
{
	unsigned int demodCount, demodLen;
	
	int64_t f;
	unsigned int countVote, voteMark, voteSpace;
	unsigned char curbit;
	
	/* Copy what was left from last time into the afsk decoding buffer. */
	memcpy( fullBuffer, demodState.remainBuff, demodState.remainCount*sizeof(int32_t) );
	/* Copy what has newly been recevied into the afsk buffer. */
	memcpy( &fullBuffer[demodState.remainCount], z_buffer, length*sizeof(int32_t) );
	/* Record how much we have just placed into our buffer. */
	demodLen = demodState.remainCount + length; 
	
	/* Compare the input signal with the expected 1k2 / 2k2 waves representing 1 and 0. */
	for( demodCount=0 ; (demodLen-demodCount)>=DEMOD_CORRELATION_WINDOW_SIZE ; demodCount+=DEMOD_SUBSAMP )
	{
		 /* Guess the input bit. */
		f = fsqr(mac(&fullBuffer[demodCount], corr_mark_i, DEMOD_CORRELATION_WINDOW_SIZE)) +
			fsqr(mac(&fullBuffer[demodCount], corr_mark_q, DEMOD_CORRELATION_WINDOW_SIZE)) -
			fsqr(mac(&fullBuffer[demodCount], corr_space_i, DEMOD_CORRELATION_WINDOW_SIZE)) -
			fsqr(mac(&fullBuffer[demodCount], corr_space_q, DEMOD_CORRELATION_WINDOW_SIZE));
		
		/* Add the decoded bit to the input shift register. */
		demodState.dcd_shreg <<= 1;
		demodState.dcd_shreg |= (f > 0);
				
		//whenever the last decoded bit changes check if phase is PI
		//try to align with the correct phase
		if ((demodState.dcd_shreg ^ (demodState.dcd_shreg >> 1)) & 1) 
		{
			/* If we have are below half was through a phase at a bit change then add on PI/4.
			   Likewise, if we are above half way through a phase at a bit change then subtract PI/4.	
			*/
			if (demodState.sphase < (0x8000u-(SPHASEINC/2)))
				demodState.sphase += SPHASEINC/8;
			else
				demodState.sphase -= SPHASEINC/8;
		}
		
		/* Increment phase by corect amount for a single sample. */
		demodState.sphase += SPHASEINC;
		
		//whenever 2PI phase is performed, goes back to 0 (clear upper bytes)
		//process new bit
		if (demodState.sphase >= 0x10000u)
		{
			demodState.sphase &= 0xffffu; //clear upper bytes
			
			//vote to decide most likely bit
			voteMark = 0;
			voteSpace = 0;
			for( countVote=0; countVote<5; countVote++ )
			{
				if( demodState.dcd_shreg & (1<<countVote) )
					voteMark++;
				else
					voteSpace++;
			}
			demodState.lasts <<= 1;
			demodState.lasts |= (voteMark>voteSpace); //add new bit to last bits register
				
			curbit = (demodState.lasts ^ (demodState.lasts >> 1) ^ 1) & 1; //implement NRZI decoding (when encoding, bit 0 change the level, bit 1 keep the level)
			AddBitToBeDecoded(curbit);
		}
	}
	
	demodState.remainCount = (demodLen-demodCount);
	memcpy( demodState.remainBuff, &fullBuffer[demodCount], (demodLen-demodCount)*sizeof(float) );
}

//multiplier and accumulator
//  used to compute the integrations
//  for the correlation demodulator
__attribute__((optimize("O3")))
int32_t mac( const int32_t *a, const int32_t *b, unsigned int size )
{
	float sum = 0;
	unsigned int i;

	for (i = 0; i < size; i++)
		sum += (*a++) * (*b++);
	
	return sum;
}


//float square
__attribute__((optimize("O3")))
int64_t fsqr( int64_t f )
{
	return f*f;
}