#ifndef __BASE_TIMER_H__
#define __BASE_TIMER_H__


// base timer definitions
#define BASE_TMR_MODULE		(&AVR32_TC1)
#define BASE_TMR_CHANNEL	0
#define BASE_TMR_IRQ		AVR32_TC1_IRQ0
#define	BASE_TMR_PRIORITY	AVR32_INTC_INT0


// base timer frequency
#define	BASE_TMR_FREQ		1000


// public function prototypes
void InitBaseTimer(void);
int32_t GetBaseTimer(void);


#endif	// __BASE_TIMER_H__