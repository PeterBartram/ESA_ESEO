#include <asf.h>
#include <math.h>
#include "intc.h"
#include "dacifb.h"
#include "pdca.h"
#include "conf_clock.h"
#include "modulator.h"
#include "DACout.h"
#include "FilterLookupTables.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "task.h"

#define FALSE 0
#define TRUE 1

/* Data packet defines. */
#define BLOCK_SIZE 256			// Size of non-FEC data block
#define FEC_BLOCK_SIZE 650		// Size of FEC block after being FEC'd
#define PREAMBLE_SIZE 768		// Number of bit times for preamble - 5 seconds = 6000 bits, 5200 FEC bits + 32 sync vector bits = 5232, 6000 - 5232 = 768 preamble bits
#define SYNC_VECTOR 0x1acffc1d	// Standard CCSDS sync vector
#define SYNC_VECTOR_SIZE 32		// Standard CCSDS sync vector

/* General DAC defines */
#define DAC_SAMPLE_RATE					19200
#define ONE_SECOND						1000000

/* 1K2 defines */
#define DAC_BIT_RATE_1K2				1200
#define DAC_SAMPLES_PER_BIT_1K2			(DAC_SAMPLE_RATE/DAC_BIT_RATE_1K2)
#define DAC_OUT_SAMPLING_TIME_1K2		(ONE_SECOND/DAC_BIT_RATE_1K2/DAC_SAMPLES_PER_BIT_1K2)
#define DAC_HISTORY_BIT_MASK_1K2		0xFF	/* 8 bit history */

/* 4K8 defines */
#define DAC_BIT_RATE_4K8				4800
#define DAC_SAMPLES_PER_BIT_4K8			(DAC_SAMPLE_RATE/DAC_BIT_RATE_4K8)
#define DAC_OUT_SAMPLING_TIME_4K8		(ONE_SECOND/DAC_BIT_RATE_4K8/DAC_SAMPLES_PER_BIT_4K8)
#define DAC_HISTORY_BIT_MASK_4K8		0xF	/* 4 bit history */

#define SYNC_VECTOR_MSB_MASK 0x80000000UL	// Mask used to take the MSB of the sync vector

/* Defines the three different transmission states that we can expected to be in. */
typedef enum 
{
	E_PREAMBLE = 0,			// This is the tone transmission.
	E_SYNC = 0x0F,			// Send the CCSDS sync vector.
	E_FEC = 0xF0			// Send the FECed data.
}TRANSMISSION_STAGE;
	
/* Output buffer from the FEC block. */
uint16_t outputBufferCount = 0;
uint8_t * outputBuffer = 0;

// private function prototypes
static void IRQDACoutPdca(void);
static uint16_t TxGetNextBit(void);
static void InitialiseDACModule(void);
static void EnableDACModule(uint8_t z_samplingTime);
static void InitialiseDMAModule(uint32_t overSamplingRate);
static void DisableDMAModule(void);

static uint16_t DACSampleOutputBuffer[2][DAC_SAMPLES_PER_BIT_1K2]; /* 1K2 uses the most samples per bit so that defines the size of this buffer. */
static uint8_t transmissionComplete = 0;						   /* Flag marking that we have finished a transmission */

/* Filter values to use - defaults to low power 1K2 mode */
int16_t	 * p_filter = FilterLookupTable_1K2_LowPower;
uint32_t samplesPerBit = DAC_SAMPLES_PER_BIT_1K2;
uint32_t historyBitMask = DAC_HISTORY_BIT_MASK_1K2;
TRANSMISSION_MODE transmissionMode = MODE_1K2_LOW_POWER;

/* Initialise the DAC ready for transmission. */
void InitialiseDAC(void)
{
	InitialiseDACModule();
	EnableDACModule(DAC_OUT_SAMPLING_TIME_1K2);
	InitialiseDMAModule(DAC_SAMPLES_PER_BIT_1K2);
}

/* Perform initalisation of the DAC hardware registers. */
static void InitialiseDACModule(void)
{
	/* Register the interrupt handler to be called when a DAC timeout occurs. */
	INTC_register_interrupt(&IRQDACoutPdca, DAC_OUT_PDCA_IRQ, DAC_OUT_PDCA_PRIORITY);
	
	/* Configure the GPIO map for the DAC module. */
	const gpio_map_t DACIFB_GPIO_MAP = {
		{AVR32_DACREF_PIN, AVR32_DACREF_FUNCTION},
		{AVR32_ADCREFP_PIN, AVR32_ADCREFP_FUNCTION},	/** Why are ADC pins being set here?? */
		{AVR32_ADCREFN_PIN, AVR32_ADCREFN_FUNCTION},
		{DAC_OUT_PIN, DAC_OUT_FUNCTION}
	};
	
	/* Enable the required pins for the DAC. */
	gpio_enable_module(DACIFB_GPIO_MAP, sizeof(DACIFB_GPIO_MAP)/sizeof(DACIFB_GPIO_MAP[0]));
	
	/* Configure the DAC interface behavior electrically. */
	dacifb_opt_t dacInterfaceConfigurationOptions;
	dacInterfaceConfigurationOptions.reference = DAC_OUT_REF;
	dacInterfaceConfigurationOptions.channel_selection = DAC_OUT_CHANNEL;
	dacInterfaceConfigurationOptions.low_power = false;
	dacInterfaceConfigurationOptions.dual = false;
	dacInterfaceConfigurationOptions.prescaler_clock_hz = DAC_OUT_PRE_CLK;
	dacifb_get_calibration_data(DAC_OUT_MODULE, &dacInterfaceConfigurationOptions, DAC_OUT_INSTANCE);
	
	/* Configure the specific DAC channel for our RF output. */
	dacifb_channel_opt_t dacChannekConfigurationOptions;
	dacChannekConfigurationOptions.auto_refresh_mode = false;
	dacChannekConfigurationOptions.trigger_mode = DACIFB_TRIGGER_MODE_TIMER;
	dacChannekConfigurationOptions.left_adjustment = false;
	dacChannekConfigurationOptions.data_shift = 0;
	dacChannekConfigurationOptions.data_round_enable = false;
	dacifb_configure(DAC_OUT_MODULE, &dacInterfaceConfigurationOptions, CLK_PBA_F);
	dacifb_configure_channel(DAC_OUT_MODULE, DAC_OUT_CHANNEL, &dacChannekConfigurationOptions, DAC_OUT_PRE_CLK);
}

/* Enable the DAC - must be configured first! */
static void EnableDACModule(uint8_t z_samplingTime)
{
	/* Enable the DAC channel */
	dacifb_start_channel(DAC_OUT_MODULE, DAC_OUT_CHANNEL, CLK_PBA_F);
	/* Configure the trigger operation for DAC interrupts. */
	dacifb_reload_timer(DAC_OUT_MODULE, DAC_OUT_CHANNEL, z_samplingTime, DAC_OUT_PRE_CLK);	
}

/* Initialise the DMA module */
static void InitialiseDMAModule(uint32_t z_overSamplingRate)
{
	/* Configure the DMA channel for a single transfer. */
	pdca_channel_options_t DMAConfigurationOptions;
	DMAConfigurationOptions.addr = (void*)DACSampleOutputBuffer;			/* Address to copy from */
	DMAConfigurationOptions.pid = DAC_OUT_PDCA_PID;							/* Write to DAC#1 transmit registers. */
	DMAConfigurationOptions.size = z_overSamplingRate;						/* Write 16 samples */
	DMAConfigurationOptions.transfer_size = PDCA_TRANSFER_SIZE_HALF_WORD;	/* Copy across data in 16 bit half words. */
	DMAConfigurationOptions.r_addr = DACSampleOutputBuffer + 1;
	DMAConfigurationOptions.r_size = z_overSamplingRate;
	
	/* Initialise the DMA transfer. */
	pdca_init_channel(DAC_OUT_PDCA_CHANNEL, &DMAConfigurationOptions);
	
	/* Allow the interrupt countdown timer to be reloaded after it reaches zero. */
	pdca_enable_interrupt_reload_counter_zero(DAC_OUT_PDCA_CHANNEL);
	
	/* Enable this DMA channel. */
	pdca_enable(DAC_OUT_PDCA_CHANNEL);
}

/* Disable the DMA module. */
static void DisableDMAModule(void)
{
	/* Disable this DMA channel. */
	pdca_disable(DAC_OUT_PDCA_CHANNEL);
}

/* This function looks at the transmissionModeChange value and changes the transmission mode if appropriate. */
void ChangeMode(TRANSMISSION_MODE z_mode)
{
	/* If we have been asked to change mode. */
	if(z_mode != transmissionMode)
	{
		/* Configure the transmission based upon requested change. */
		switch(z_mode)
		{
			case MODE_1K2_LOW_POWER:
				{
					/* Interrupts need to be disabled to prevent race condition */
					taskENTER_CRITICAL();
					DisableDMAModule();
					EnableDACModule(DAC_OUT_SAMPLING_TIME_1K2);
					InitialiseDMAModule(DAC_SAMPLES_PER_BIT_1K2);
					taskEXIT_CRITICAL();
					p_filter = FilterLookupTable_1K2_LowPower;
					samplesPerBit = DAC_SAMPLES_PER_BIT_1K2;
					historyBitMask = DAC_HISTORY_BIT_MASK_1K2;
					transmissionMode = MODE_1K2_LOW_POWER;
				}
				break;
			case MODE_1K2_HIGH_POWER:
				{
					/* Interrupts need to be disabled to prevent race condition */
					taskENTER_CRITICAL();					
					DisableDMAModule();
					EnableDACModule(DAC_OUT_SAMPLING_TIME_1K2);
					InitialiseDMAModule(DAC_SAMPLES_PER_BIT_1K2);
					taskEXIT_CRITICAL();
					p_filter = FilterLookupTable_1K2_HighPower;
					samplesPerBit = DAC_SAMPLES_PER_BIT_1K2;
					historyBitMask = DAC_HISTORY_BIT_MASK_1K2;	
					transmissionMode = MODE_1K2_HIGH_POWER;
				}
				break;
			case MODE_4K8_LOW_POWER:
				{
					/* Interrupts need to be disabled to prevent race condition */
					taskENTER_CRITICAL();					
					DisableDMAModule();
					EnableDACModule(DAC_OUT_SAMPLING_TIME_4K8);
					InitialiseDMAModule(DAC_SAMPLES_PER_BIT_4K8);
					taskEXIT_CRITICAL();
					p_filter = FilterLookupTable_4K8_LowPower;
					samplesPerBit = DAC_SAMPLES_PER_BIT_4K8;
					historyBitMask = DAC_HISTORY_BIT_MASK_4K8;	
					transmissionMode = MODE_4K8_LOW_POWER;
				}
				break;
			case MODE_4K8_HIGH_POWER:
				{
					/* Interrupts need to be disabled to prevent race condition */
					taskENTER_CRITICAL();					
					DisableDMAModule();
					EnableDACModule(DAC_OUT_SAMPLING_TIME_4K8);
					InitialiseDMAModule(DAC_SAMPLES_PER_BIT_4K8);
					taskEXIT_CRITICAL();
					p_filter = FilterLookupTable_4K8_HighPower;
					samplesPerBit = DAC_SAMPLES_PER_BIT_4K8;
					historyBitMask = DAC_HISTORY_BIT_MASK_4K8;
					transmissionMode = MODE_4K8_HIGH_POWER;
				}
				break;
			/* Invalid command - ignore it and carry on in current mode. */
			default: return;
		}
	}
}

/*
	The is the interrupt handler for the DAC and takes the appropriate bit from the data to be transmitted and updated the
    filter history table, it then does a look up for the desired filter output values and adds the 16 values to the DAC buffer.
   */
__attribute__((__interrupt__))
static void IRQDACoutPdca(void)	//This interrupt waits for DAC buffer to be empty
{
	// This is called each bit time, ie 1200bps
	static int bufferIndex = 0; // Current buffer to load (may be 0 or 1)
	static uint16_t u16BitHistory = 0;
	uint16_t i;
	
	uint16_t bBit=TxGetNextBit(); // Get the next bit
	int16_t * p_filterLookupTableSample = p_filter + (u16BitHistory * samplesPerBit); // Nice and quick, use a pointer to the table
	uint16_t *pu16DACBuf = DACSampleOutputBuffer[bufferIndex]; // Pointer to current DAC DMA fill buffer
		
	u16BitHistory=((u16BitHistory<<1) | bBit) & historyBitMask; // Keep a history of the bits we've used for the FIR lookup table

	// Copy the data from the lookup table into the DAC output buffer
	for (i = 0; i < samplesPerBit; i++)
	{
		pu16DACBuf[i] = p_filterLookupTableSample[i];
	}

	pdca_reload_channel(DAC_OUT_PDCA_CHANNEL, pu16DACBuf, samplesPerBit); // Get the DMA controller ready to auto reload for the next time it's finished

	bufferIndex=(bufferIndex+1) & 1; // Flip the fill buffer for next time round
}

/* 
	Returns the next bit from the FECed data, preamble or sync depending on state.
*/
static uint16_t TxGetNextBit(void) 
{
	static TRANSMISSION_STAGE  eState = E_PREAMBLE;	// Transmission mode enum.	
	static uint8_t bitIndex;						// Bit index for FECed data.
	static uint16_t byteIndex;						// Byte index for FECed data.
	uint16_t bitValue = 0;							// Calculated value of our MSB to transmit.
	static uint16_t outputValue = FALSE;				// State of differential output.
	static uint16_t preambleCount = 0;				// Number of preamble bits we have sent.
	static uint8_t syncVectorCount = 0;				// Number of sync vector bits we have sent.
	static uint32_t syncVector;						// Contains the sync vector (actively shifted so cant just define this).
	
	/* State machine for our transmission stages */
	switch(eState)
	{
		case E_PREAMBLE:
		{
			preambleCount++;
		
			/* Are we finished with the preamble? Then move onto the sync vector */
			if (preambleCount >= PREAMBLE_SIZE)
			{
				eState = E_SYNC;
				syncVectorCount = 0;
				syncVector = SYNC_VECTOR;
			}
		}
		break;
		case E_SYNC:
		{
			bitValue = (syncVector & SYNC_VECTOR_MSB_MASK); // MSB first is CCSDS standard as I read it
			syncVector<<=1;
			syncVectorCount++;
			
			/* Are we finished with the sync vector? Then move onto the FEC data. */
			if (syncVectorCount >= SYNC_VECTOR_SIZE)
			{
				eState=E_FEC;
				byteIndex=0;
				bitIndex=0;
			}
		}
		break;
		case E_FEC:
		{
			if(outputBuffer != 0 && outputBufferCount != 0)
			{
				bitValue=(outputBuffer[byteIndex]>>(7-bitIndex) & 0x01)!=0; // Note that MSB first is the AO-40 standard

				bitIndex++;
				if (bitIndex>=8)
				{
					bitIndex=0;
					byteIndex++;
				
					/* Are we finished with the FECed data? Then we can reset. */
					if (byteIndex >= FEC_BLOCK_SIZE)
					{
						/* Reset our control variables for the next iteration */
						eState=E_PREAMBLE;
						preambleCount = 0;
						outputBufferCount = 0;
						//********************* Here you could trigger something EXTERNAL to load a new FEC block
						// This is inside the ISR, so no long running nonsense here!
						// The next phase is preamble, so as long as your FEC block is set up in the preamble time, all is good (640ms)
						transmissionComplete = 1;
					}
				}
			}
			else
			{
				/* No data in the output buffer so we reset. - Shouldn't even happen! */
				eState=E_PREAMBLE;
				preambleCount = 0;
				outputBufferCount = 0;	
				transmissionComplete = 1;			
			}
		}
		break;
		default:
		{
			/* Single event upset has occurred - cut our losses and drop the packet */
			eState = E_PREAMBLE;
			preambleCount = 0;
			outputBufferCount = 0;	
			transmissionComplete = 1;	
		}
	}

	/* We're differential on zero bits, so change on zero. */
	if (bitValue == 0)
	{
		outputValue = !outputValue;
	}
	return outputValue;
}

/* Function tells the DAC that there is a packet waiting for it in the buffer provided */
void DACOutputBufferReady (uint8_t * z_packet, uint16_t z_size)
{
	if(((outputBufferCount + z_size) <= DAC_OUT_BUFF_SIZE) && z_packet)
	{
		outputBufferCount = outputBufferCount + z_size;
		outputBuffer = z_packet;
	}
}

/* 
   Returns the size of the data buffer
   IMPORTANT: use this to check if the buffer is empty before
   transmitting a new packet 
*/
uint32_t DACoutBuffSize(void)
{
	return outputBufferCount;
}

/* This function returns true if we have finished transmitting a block - calling this function will clear the transmission complete flag. */
uint8_t TransmitReady(void)
{
	/* Pretty sure we need some semaphore goodness around here!!!! */
	uint8_t retVal = transmissionComplete;
	
	transmissionComplete = 0;
	
	return retVal;
}