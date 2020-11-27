/*
 * Watchdog.h
 *
 * Created: 22-Aug-16 8:06:26 PM
 *  Author: Pete
 */ 


#ifndef WATCHDOG_H_
#define WATCHDOG_H_

#define NUMBER_OF_THREADS_MANAGED 5

typedef enum _THREAD_ID
{
	CAN_OPEN_THREAD = 0,
	DOWNLINK_THREAD,
	UPLINK_THREAD,
	TELEMETRY_THREAD,
	PAYLOAD_DATA_THREAD	
}THREAD_ID;

void InitialiseWatchdog(void);
void RegisterThreadWithManager(THREAD_ID z_thread);
void ProvideThreadHeartbeat(THREAD_ID z_thread);
void UpdateWatchdog(void);
void DisableWatchdog(void);
void DisableThreadHeartbeat(THREAD_ID z_thread);
void KickUplinkCommandWatchdog(void);

#endif /* WATCHDOG_H_ */