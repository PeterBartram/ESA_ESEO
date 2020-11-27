#ifndef TIMERS_H
#define TIMERS_H

#define BaseType_t long

#include "stdint.h"
#include "projdefs.h"

/* IDs for commands that can be sent/received on the timer queue.  These are to
be used solely through the macros that make up the public software timer API,
as defined below. */
#define tmrCOMMAND_START					0
#define tmrCOMMAND_STOP						1
#define tmrCOMMAND_CHANGE_PERIOD			2
#define tmrCOMMAND_DELETE					3

/*-----------------------------------------------------------
 * MACROS AND DEFINITIONS
 *----------------------------------------------------------*/

 /**
 * Type by which software timers are referenced.  For example, a call to
 * xTimerCreate() returns an xTimerHandle variable that can then be used to
 * reference the subject timer in calls to other software timer API functions
 * (for example, xTimerStart(), xTimerReset(), etc.).
 */
typedef void * xTimerHandle;

/* Define the prototype to which timer callback functions must conform. */
typedef void (*tmrTIMER_CALLBACK)( xTimerHandle xTimer );

typedef uint32_t StaticTimer_t;

#define xTimerCreate(name, period, reload, id, func)			TimerCreate(name, period, reload, id, func)
#define xTimerStart(xTimer, xBlockTime)							TimerStart(xTimer, xBlockTime)
#define xTimerStop(xTimer, xBlockTime)							TimerStop(xTimer, xBlockTime)	
#define xTimerDelete(xTimer, xBlockTime)						TimerDelete(xTimer, xBlockTime)	
#define xTimerReset(xTimer, xBlockTime)							TimerReset(xTimer, xBlockTime)	
#define xTimerCreateStatic(name, period, reload, id, func, mem) TimerCreate(name, period, reload, id, func)

xTimerHandle TimerCreate(const char *, uint32_t, uint32_t, void *, tmrTIMER_CALLBACK);
uint32_t TimerStart(xTimerHandle z_timer, uint32_t z_delay);
uint32_t TimerStop(xTimerHandle z_timer, uint32_t z_delay);
uint32_t TimerDelete(xTimerHandle z_timer, uint32_t z_delay);
uint32_t TimerReset(xTimerHandle z_timer, uint32_t z_delay);

BaseType_t xTimerIsTimerActive(xTimerHandle);

#endif /* TIMERS_H */



