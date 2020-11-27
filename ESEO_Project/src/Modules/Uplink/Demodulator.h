#ifndef DEMODULATOR_H_
#define DEMODULATOR_H_

#include <stdint.h>

//afsk parameters
#define DEMOD_FREQ_MARK		1200												//mark (1) frequency in Hz
#define DEMOD_FREQ_SPACE	2200												//space (0) frequency in Hz
#define DEMOD_FREQ_SAMP		11025												//sampling rate in samples/s
#define DEMOD_BAUD			1200												//baud rate in bps
#define DEMOD_SUBSAMP		1													//distance (in samples) between sub-samples
#define DEMOD_CORRELATION_WINDOW_SIZE		((int)(DEMOD_FREQ_SAMP/DEMOD_BAUD))	//correlation demodulator window length (number of samples in a bit period)
#define DEMOD_LENGTH		500
#define DEMOD_BUFF_LEN		(DEMOD_LENGTH + DEMOD_CORRELATION_WINDOW_SIZE)

typedef struct{
	unsigned int dcd_shreg;
	unsigned int sphase;
	unsigned int lasts;
	int32_t remainBuff[DEMOD_CORRELATION_WINDOW_SIZE];
	unsigned int remainCount;
}DEMODULATOR_STATE;

void AFSKInit(void);
void AFSKDemod(int32_t * z_buffer, uint32_t length);



#endif /* DEMODULATOR_H_ */