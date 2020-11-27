#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include <string.h>
#include "Watchdog.h"
#include "CANODInterface.h"
#include "CANObjectDictionary.h"
#include "wdt.h"
#include "power_clocks_lib.h"
#include "cycle_counter.h"

#define WATCHDOG_ACCESS_TICKS_TO_WAIT 10
#define AUTO_RELOAD 1
#define HOURS_TO_TICKS	 3600000
#define TICKS_TO_HOURS	 HOURS_TO_TICKS
#define SECONDS_TO_TICKS 1000
#define TICKS_TO_US		 1000

typedef struct _THREAD_CONTROL
{
	uint8_t threadBeingManaged[NUMBER_OF_THREADS_MANAGED];
	uint8_t threadActive[NUMBER_OF_THREADS_MANAGED];
}THREAD_CONTROL;

/* Cannot do this with a memset in the initialise function as sat ops is initialised after 
other threads and it would clear the initialisations performed in registering each thread. */ 
static THREAD_CONTROL control = 
{
	{0,0,0,0,0},		// We dont know which threads we are going to have to manage.
	{1,1,1,1,1}			// Assume all threads are good for the first watchdog period - let the system settle. 
};

wdt_opt_t wdtOptions = {
	.dar   = false,								// After a watchdog reset, the WDT will still be enabled.
	.mode  = WDT_BASIC_MODE,					// The WDT is in basic mode, only PSEL time is used.
	.sfv   = false,								// WDT Control Register is not locked.
	.fcd   = false,								// The flash calibration will be redone after a watchdog reset.
	.cssel = WDT_CLOCK_SOURCE_SELECT_RCSYS,     // Select the system RC oscillator (RCSYS) as clock source.
};

static SemaphoreHandle_t heartbeatSemaphore;
static StaticSemaphore_t heartbeatSemaphoreStorage;
static StaticTimer_t watchdogTimer;
static xTimerHandle  watchdogTimerHandle;
static StaticTimer_t uplinkWatchdogTimer;
static xTimerHandle  uplinkWatchdogTimerHandle;
static uint8_t uplinkUpdate = 0;
static uint8_t kickWatchdog = 0;
static uint8_t failure = 0;
static uint32_t watchdogTimeOut = 0;
static uint32_t uplinkWatchdogTime = 0;

static void watchdogTimerCallback(xTimerHandle xTimer);
static void UplinkCommandWatchdogCallback(xTimerHandle xTimer);
static uint32_t CalculateCommandTimeOutTimeRemaining(void);

/* Configure the watchdog time out period and start the watchdog itself. */
void InitialiseWatchdog(void)
{
	
	kickWatchdog = 0;
	uplinkWatchdogTime = 0;
		
	/* Get the watchdog time out. */
	GetCANObjectDataRaw(AMS_OBC_HB_WDT, &watchdogTimeOut);
	
	/* Limit the timeout to 10 seconds minimum. */
	watchdogTimeOut = (watchdogTimeOut < 10) ? 10 : watchdogTimeOut;
	
	/* Convert to ticks to apply correct time out to FreeRTOS timer function. */
	watchdogTimeOut *= SECONDS_TO_TICKS;
	
	heartbeatSemaphore = xSemaphoreCreateRecursiveMutexStatic(&heartbeatSemaphoreStorage);
	watchdogTimerHandle = xTimerCreateStatic("WatchdogTimer", watchdogTimeOut, AUTO_RELOAD, 0, &watchdogTimerCallback, &watchdogTimer);
	xTimerStart(watchdogTimerHandle, 0);
		
	/* Convert from tick into to uS to apply correct system time out. */
	watchdogTimeOut *= TICKS_TO_US;
	wdtOptions.us_timeout_period = watchdogTimeOut;
	/* Start the watchdog. */
	wdt_enable(&wdtOptions);
	
	/* Get the uplink watchdog time out. */
	GetCANObjectDataRaw(AMS_OBC_LC_WDT, &uplinkWatchdogTime);	
	
	/* Limit the watchdog to a minimum period of 5 hours. */
	uplinkWatchdogTime = (uplinkWatchdogTime < 5) ? 5 : uplinkWatchdogTime;
	
	/* Convert from hours into ticks (mS) to use as a time out period. */
	uplinkWatchdogTime *= HOURS_TO_TICKS;
	
	/* Start a command watchdog timer. */
	uplinkWatchdogTimerHandle = xTimerCreateStatic("UplinkWatchdog", uplinkWatchdogTime, AUTO_RELOAD, 0, &UplinkCommandWatchdogCallback, &uplinkWatchdogTimer);
	xTimerStart(uplinkWatchdogTimerHandle, 0);	
}

/* Just checks that we have received a heartbeat from each thread within the watchdog time out period
   and either kicks the watchdog or sets the watchdog to trigger immediately if we haven't had all heartbeats */
void UpdateWatchdog(void)
{
	uint8_t i = 0;
	QueueHandle_t commandQueue = 0;
	uint32_t threadsRunning = 0;
	uint32_t data = 0;
				
	/* If we have failed then just let the watchdog time out itself. */
	if(failure == 0)
	{
		/* Time to check if we need to let the watchdog expire. */
		if(kickWatchdog != 0)
		{
			if(xSemaphoreTakeRecursive(heartbeatSemaphore, WATCHDOG_ACCESS_TICKS_TO_WAIT) != 0)
			{	
				for(i = 0; i < NUMBER_OF_THREADS_MANAGED; i++)
				{					
					if(control.threadBeingManaged[i])
					{
						if(control.threadActive[i] == 0)
						{
							/* We have not received a heartbeat from the thread in question so we need to reset. */
							failure = 1;
							/* Reset the mcu as fast as possible. */
							wdt_reset_mcu();
						}
						else
						{
							/* Create a single variable with flags for each thread in it. */
							threadsRunning |= (1 << i);
						}
						
					}
					/* Reset the thread heartbeat indicator for next time. */
					control.threadActive[i] = 0;
				}
				kickWatchdog = 0;
				xSemaphoreGiveRecursive(heartbeatSemaphore);
			}
			
			/* Get the CAN command queue handle. */
			commandQueue = GetCommandQueueHandle();
						
			/* Update the debug information in the CAN OD */
			if(commandQueue != 0)
			{
				uint8_t buffer[CAN_QUEUE_ITEM_SIZE];
				uint32_t timeRemainingCommandUplink = 0;
				/* Record that this data is from the uplink. */
				buffer[PACKET_SOURCE_INDEX] = ON_TARGET_DATA;
				/* Add the packet length to the queue packet. */
				buffer[PACKET_SIZE_INDEX] = 5;
				buffer[OBJECT_DICTIONARY_ID_INDEX] = AMS_OBC_THREAD_HB;
				memcpy(&buffer[PACKET_DATA_INDEX], &threadsRunning, sizeof(threadsRunning));
				xQueueSend(commandQueue, buffer, 0);
				
				/* Add the command uplink time remaining to the CAN OD. */
				buffer[OBJECT_DICTIONARY_ID_INDEX] = AMS_OBC_CMD_WDT_TIME_REM;
				timeRemainingCommandUplink = CalculateCommandTimeOutTimeRemaining();
				memcpy(&buffer[PACKET_DATA_INDEX], &timeRemainingCommandUplink, sizeof(timeRemainingCommandUplink));
				xQueueSend(commandQueue, buffer, 0);				
			}
		}
		/* Clear the watchdog as no problems detected. */
		wdt_clear();
	}
}

/* Register a thread with the watchdog manager - threads will 
   not be managed if this function hasnt been called for them */
void RegisterThreadWithManager(THREAD_ID z_thread)
{
	if(z_thread < NUMBER_OF_THREADS_MANAGED)
	{
		control.threadBeingManaged[z_thread] = 1;
	}
}

/* Provide a heartbeat for a single thread - must be called for every thread within the heartbeat period. */
void ProvideThreadHeartbeat(THREAD_ID z_thread)
{
	if(xSemaphoreTakeRecursive(heartbeatSemaphore, WATCHDOG_ACCESS_TICKS_TO_WAIT) != 0)
	{
		if(z_thread < NUMBER_OF_THREADS_MANAGED)
		{
			control.threadActive[z_thread] = 1;
		}
		xSemaphoreGiveRecursive(heartbeatSemaphore);
	}
}

/* Called when the heartbeat period has expired. */
static void watchdogTimerCallback(xTimerHandle xTimer)
{
	kickWatchdog = 1;
}

/* Disables the heartbeat watchdog - must be called within 10 seconds of start up if we are constantly watchdogging */
void DisableWatchdog(void)
{
	memset(&control, 0, sizeof(control));
}

/* Disable heartbeat watchdog for a single thread */
void DisableThreadHeartbeat(THREAD_ID z_thread)
{
	if(z_thread < NUMBER_OF_THREADS_MANAGED)
	{
		control.threadBeingManaged[z_thread] = 0;
	}
}

/* Kick the command watchdog so that it doesn't reset */
void KickUplinkCommandWatchdog(void)
{
	if(uplinkWatchdogTimerHandle != 0)
	{
		xTimerReset(uplinkWatchdogTimerHandle, WATCHDOG_ACCESS_TICKS_TO_WAIT);
	}
}

/* If this ever gets called then we have timed out. */
static void UplinkCommandWatchdogCallback(xTimerHandle xTimer)
{
	failure = 1;	
}

static uint32_t CalculateCommandTimeOutTimeRemaining(void)
{
	uint32_t timeRemaining = 0;
	
	if(uplinkWatchdogTimerHandle != 0)
	{
		timeRemaining  = xTimerGetExpiryTime(uplinkWatchdogTimerHandle) - xTaskGetTickCount();
		timeRemaining /= TICKS_TO_HOURS;
	}
	return timeRemaining;
}