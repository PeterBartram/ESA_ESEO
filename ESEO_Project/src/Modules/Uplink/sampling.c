//headers
#include <asf.h>
#include "conf_clock.h"
#include "intc.h"
#include "tc.h"
#include "adcifa.h"
#include "sampling.h"
#include <string.h>

#define SAMPLE_BUFFER_SIZE 2048

int16_t samplingBuffer[SAMPLE_BUFFER_SIZE];

uint32_t bufferCount;
uint32_t bufferInputIndex = 0;
uint32_t bufferOutputIndex = 0;

//private function prototypes
void InitSamplingTimer(void);
static void IRQSamplingTimer(void);
void InitSamplingAdc(void);
static void IRQSamplingAdc(void);
static void InitialiseSamplingBuffer(void);

//adcifa sequencer general configuration structure
adcifa_sequencer_opt_t adcifa_sequence_opt = {
	1, //1 conversion (1 channel)
	ADCIFA_SRES_12B, //12 bits resolution
	ADCIFA_TRGSEL_SOFT, //software trigger source
	ADCIFA_SOCB_ALLSEQ, //read all sequencer when trigged
	ADCIFA_SH_MODE_OVERSAMP, //oversampling enabled
	ADCIFA_HWLA_NOADJ, //hwla disabled
	ADCIFA_SA_NO_EOS_SOFTACK //enable acknowledge
};


////sample buffer & counter
//volatile int16_t samplingBuffer[SAMPLING_BUFF_SIZE];
//volatile uint32_t samplingBufferSize = 0;


//timer initialization
void InitSamplingTimer(void)
{
	tc_waveform_opt_t waveform_opt;
	tc_interrupt_t tc_interrupt;
	
	waveform_opt.channel = TC_SAMPLING_CHANNEL;	//uses channel 0
	waveform_opt.bswtrg = TC_EVT_EFFECT_NOOP; //no sw effect on TIOB
	waveform_opt.beevt = TC_EVT_EFFECT_NOOP; //no evt effect on TIOB
	waveform_opt.bcpc = TC_EVT_EFFECT_NOOP; //no RC cp effect on TIOB
	waveform_opt.bcpb = TC_EVT_EFFECT_NOOP; //no RB cp effect on TIOB
	waveform_opt.aswtrg = TC_EVT_EFFECT_NOOP; //no sw effect on TIOA
	waveform_opt.aeevt = TC_EVT_EFFECT_NOOP; //no evt effect on TIOA
	waveform_opt.acpc = TC_EVT_EFFECT_NOOP; //no RC cp effect on TIOA
	waveform_opt.acpa = TC_EVT_EFFECT_NOOP; //no RA cp effect on TIOA
	waveform_opt.wavsel = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER; //up count reset on RC
	waveform_opt.enetrg = false; //no trigger on external events
	waveform_opt.eevt = 0; //external event TIOB (not used, disabled)
	waveform_opt.eevtedg = TC_SEL_NO_EDGE; //no edge triggers external event
	waveform_opt.cpcdis = false; //counter not disabled when CP match occur
	waveform_opt.cpcstop = false; //clock not stopped when CP match occur
	waveform_opt.burst = false; //clock is not gated
	waveform_opt.clki = false; //increment on rising edge
	waveform_opt.tcclks = TC_CLOCK_SOURCE_TC2; //tmr clk = PB clk / 2
	
	tc_interrupt.etrgs = 0;
	tc_interrupt.ldrbs = 0;
	tc_interrupt.ldras = 0;
	tc_interrupt.cpcs = 1; //only interrupt on RC match
	tc_interrupt.cpbs = 0;
	tc_interrupt.cpas = 0;
	tc_interrupt.lovrs = 0;
	tc_interrupt.covfs = 0;
	
	tc_init_waveform( TC_SAMPLING_MODULE, &waveform_opt ); //init timer in waveform mode
	tc_write_rc( TC_SAMPLING_MODULE, TC_SAMPLING_CHANNEL, CLK_PBC_F/2/SAMPLING_RATE ); //setup load value for RC
	tc_configure_interrupts( TC_SAMPLING_MODULE, TC_SAMPLING_CHANNEL, &tc_interrupt ); //init interrupts
	tc_start( TC_SAMPLING_MODULE, TC_SAMPLING_CHANNEL ); //start timer

	return;
}


//timer interrupt handler
__attribute__((__interrupt__))
static void IRQSamplingTimer(void)
{
	//gpio_set_gpio_pin(LED0_GPIO);
	
	//clear flag by reading sr
	tc_read_sr( TC_SAMPLING_MODULE, TC_SAMPLING_CHANNEL );

	//trigger adc
	//if( samplingBufferSize < SAMPLING_BUFF_SIZE )
		adcifa_start_sequencer( ADC_SAMPLING_MODULE, 0 );

	//gpio_clr_gpio_pin(LED0_GPIO);
	return;
}


//adc initialization
void InitSamplingAdc(void)
{
	//adcifa gpio map
	static const gpio_map_t ADCIFA_GPIO_MAP = {
		{AVR32_ADCREF1_PIN,AVR32_ADCREF1_FUNCTION},
		{AVR32_ADCREFP_PIN,AVR32_ADCREFP_FUNCTION},
		{AVR32_ADCREFN_PIN,AVR32_ADCREFN_FUNCTION},
		{ADC_SAMPLING_PIN,ADC_SAMPLING_FUNCTION}
	};
	
	//adcifa general module configuration structure
	adcifa_opt_t adc_config_t;
	adc_config_t.frequency = 1000000; //adc frequency
	adc_config_t.reference_source = ADCIFA_ADCREF1; //reference source
	adc_config_t.sample_and_hold_disable = false; //disable sample and hold time
	adc_config_t.single_sequencer_mode = false; //single sequencer mode
	adc_config_t.free_running_mode_enable = false; //free running mode
	adc_config_t.sleep_mode_enable = false; //sleep mode
	adc_config_t.mux_settle_more_time = false; //multiplexer settle time
	
	//adcifa sequencer 0 channel configuration structure (only 1 channel used)
	adcifa_sequencer_conversion_opt_t adcifa_sequence_conversion_opt_seq0;
	adcifa_sequence_conversion_opt_seq0.channel_p = ADC_SAMPLING_INP; //positive channel
	adcifa_sequence_conversion_opt_seq0.channel_n = ADC_SAMPLING_INN; //negative channel
	adcifa_sequence_conversion_opt_seq0.gain = ADC_SAMPLING_GAIN; //gain
	
	//configure
	gpio_enable_module( ADCIFA_GPIO_MAP, sizeof(ADCIFA_GPIO_MAP)/sizeof(ADCIFA_GPIO_MAP[0]) ); //gpio enable & map
	adcifa_get_calibration_data( ADC_SAMPLING_MODULE, &adc_config_t ); //get factory general module configuration
	adcifa_configure( ADC_SAMPLING_MODULE, &adc_config_t, CLK_PBC_F ); //setup adcifa core
	adcifa_configure_sequencer( ADC_SAMPLING_MODULE, 0, &adcifa_sequence_opt, &adcifa_sequence_conversion_opt_seq0 ); //setup sequencer 0
	adcifa_enable_interrupt( ADC_SAMPLING_MODULE, AVR32_ADCIFA_IER_SEOS0_MASK ); //enable interrupt
	
	InitialiseSamplingBuffer();

	return;
}


//adc interrupt handler
__attribute__((__interrupt__))
static void IRQSamplingAdc(void)
{
	int16_t adcValue;

	if( adcifa_get_values_from_sequencer(ADC_SAMPLING_MODULE,0,&adcifa_sequence_opt,&adcValue) == ADCIFA_STATUS_COMPLETED )
	{
		//if(bufferCount < SAMPLE_BUFFER_SIZE)
		{
			UplinkInputBufferSet(adcValue);
			//samplingBuffer[samplingBufferSize] = adcValue;
			//samplingBufferSize++;
		}
	}
	
	adcifa_clear_interrupt( ADC_SAMPLING_MODULE, AVR32_ADCIFA_IMR_SEOS0_MASK );
	
	return;
}


//general sampling initialization
void InitSampling(void)
{
	INTC_register_interrupt( &IRQSamplingTimer, TC_SAMPLING_IRQ, TC_SAMPLING_PRIORITY ); //register tc int handler
	INTC_register_interrupt( &IRQSamplingAdc, ADC_SAMPLING_IRQ, ADC_SAMPLING_PRIORITY ); //register adc int handler
	
	InitSamplingTimer(); //initialize timer
	InitSamplingAdc(); //initialize adc
	
	return;
}

static void InitialiseSamplingBuffer(void)
{
	memset(&samplingBuffer[0], 0, sizeof(samplingBuffer));
	bufferCount = 0;
	bufferInputIndex = 0;
	bufferOutputIndex = 0;
}

__attribute__((optimize("O3")))
uint32_t SizeSamplingBuff(void)
{
	return bufferCount;
}

__attribute__((optimize("O3")))
void UplinkInputBufferSet(int16_t z_data)
{
	if(bufferCount < SAMPLE_BUFFER_SIZE)
	{
		bufferCount++;
	}

	if(bufferInputIndex >= SAMPLE_BUFFER_SIZE)
	{
		bufferInputIndex = 0;
	}

	samplingBuffer[bufferInputIndex] = z_data;
	bufferInputIndex++;
}

__attribute__((optimize("O3")))
uint32_t UplinkInputBufferGet(int32_t * z_data)
{
	uint32_t retVal = 0;

	if(z_data != 0 && bufferCount != 0)
	{
		if(bufferOutputIndex >= SAMPLE_BUFFER_SIZE)
		{
			bufferOutputIndex = 0;
		}
		bufferCount--;
		retVal = bufferCount;
		*z_data = samplingBuffer[bufferOutputIndex];
		bufferOutputIndex++;
	}
	return retVal;
}