#ifndef	__DAC_OUT_H__
#define __DAC_OUT_H__


// headers
#include "conf_clock.h"
#include "modulator.h"


// public function prototypes
void InitialiseDAC(void);
uint32_t DACoutBuffSize(void);
void DACOutputBufferReady (uint8_t * z_packet, uint16_t z_size);
void DACChangeModulatorMode(TRANSMISSION_MODE z_mode);
uint8_t	TransmitReady(void);
void ChangeMode(TRANSMISSION_MODE z_mode);

// dac module configuration definitions
#define DAC_OUT_MODULE			(&AVR32_DACIFB1)
#define DAC_OUT_INSTANCE		1
#define DAC_OUT_REF				DACIFB_REFERENCE_EXT
#define DAC_OUT_PIN				AVR32_DAC1B_PIN
#define DAC_OUT_FUNCTION		AVR32_DAC1B_FUNCTION
#define DAC_OUT_CHANNEL			DACIFB_CHANNEL_SELECTION_B
#define DAC_OUT_PRE_CLK			(CLK_PBA_F/32)

// pdca module configuration definitions
#define DAC_OUT_PDCA_CHANNEL	0
#define DAC_OUT_PDCA_PID		AVR32_PDCA_PID_DACIFB1_CHB_TX
#define DAC_OUT_PDCA_IRQ		AVR32_PDCA_IRQ_0
#define DAC_OUT_PDCA_PRIORITY	AVR32_INTC_INT3


// data output definitions
#define DAC_OUT_BUFF_SIZE		1024	// 1kB output buffer = 8192 bits


#endif // __DAC_OUT_H__