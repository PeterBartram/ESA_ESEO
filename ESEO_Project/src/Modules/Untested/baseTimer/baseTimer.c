// headers
#include <asf.h>
#include "tc.h"
#include "intc.h"
#include "conf_clock.h"
#include "baseTimer.h"


// global variable
int32_t baseTimerCount = 0;


// internal function prototypes
static void IRQBaseTimer(void);


// base timer initialization
void InitBaseTimer(void)
{
	tc_waveform_opt_t waveform_opt;
	tc_interrupt_t tc_interrupt;
	
	waveform_opt.channel = BASE_TMR_CHANNEL;	//uses channel 0
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
	
	INTC_register_interrupt( &IRQBaseTimer, BASE_TMR_IRQ, BASE_TMR_PRIORITY ); // register handler
	
	tc_init_waveform( BASE_TMR_MODULE, &waveform_opt ); //init timer in waveform mode
	tc_write_rc( BASE_TMR_MODULE, BASE_TMR_CHANNEL, CLK_PBA_F/2/BASE_TMR_FREQ ); //setup load value for RC
	tc_configure_interrupts( BASE_TMR_MODULE, BASE_TMR_CHANNEL, &tc_interrupt ); //init interrupts
	tc_start( BASE_TMR_MODULE, BASE_TMR_CHANNEL ); //start timer

	return;
}


// base timer interrupt handler
__attribute__((__interrupt__))
static void IRQBaseTimer(void)
{
	//clear flag by reading sr
	tc_read_sr( BASE_TMR_MODULE, BASE_TMR_CHANNEL );

	// increment counter
	baseTimerCount++;
	
	return;
}


// return counter value
int32_t GetBaseTimer(void)
{
	return baseTimerCount;
}

