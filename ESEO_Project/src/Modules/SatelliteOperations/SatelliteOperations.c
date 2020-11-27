#include "SatelliteOperations.h"
#include "OperatingModes.h"
#include "WOD.h"
#include "DefaultValues.h"
#include "Watchdog.h"


void InitialiseSatelliteOperations(SemaphoreHandle_t z_flashSemaphore)
{
	InitialiseDefaultValues(z_flashSemaphore);	// This MUST be done before InitialiseOperatingModes!
	InitialiseWOD();	
	InitialiseOperatingModes();
	InitialiseWatchdog();
}

/* Main thread function to be called by OS. */
void SatelliteOperationsThread(void)
{
	while(1)
	{
		SatelliteOperationsTask();
		vTaskDelay(100);
	}
}

/* This is the main satellite operations task */
void SatelliteOperationsTask(void)
{
	UpdateSequenceNumber();
	UpdateWODData();
	UpdateDefaultValues();
	UpdateOperatingModes();
	UpdateWatchdog();
}
