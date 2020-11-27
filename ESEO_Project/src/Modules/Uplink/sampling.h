#ifndef __SAMPLING_H__
#define __SAMPLING_H__


//headers
#include <asf.h>
#include <stdint.h>

void InitSampling(void);
uint32_t SizeSamplingBuff(void);
void	UplinkInputBufferSet(int16_t z_data);
uint32_t UplinkInputBufferGet(int32_t * z_data);

//timer definitions
#define TC_SAMPLING_MODULE		(&AVR32_TC0)
#define TC_SAMPLING_CHANNEL		0
#define TC_SAMPLING_IRQ			AVR32_TC0_IRQ0
#define TC_SAMPLING_PRIORITY	AVR32_INTC_INT1


//adcifa definitions
#define ADC_SAMPLING_MODULE		(&AVR32_ADCIFA)
#define	ADC_SAMPLING_PIN		AVR32_ADCIN5_PIN
#define ADC_SAMPLING_FUNCTION	AVR32_ADCIN5_FUNCTION
#define ADC_SAMPLING_INP		AVR32_ADCIFA_INP_ADCIN5
#define	ADC_SAMPLING_INN		AVR32_ADCIFA_INN_GNDANA
#define	ADC_SAMPLING_GAIN		ADCIFA_SHG_1
#define ADC_SAMPLING_IRQ		AVR32_ADCIFA_SEQUENCER0_IRQ
#define ADC_SAMPLING_PRIORITY	AVR32_INTC_INT1


//sampling definitions
#define SAMPLING_RATE			11025

#endif //__SAMPLING_H__