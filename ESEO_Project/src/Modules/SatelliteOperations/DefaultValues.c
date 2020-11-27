#include "SatelliteOperations.h"
#include "DefaultValues.h"
#include "flashc.h"

/* Configuration values - don't change these! */
#define SEQUENCE_NUMBER_UPDATE_PERIOD_MINUTES	2		// Period at which the sequence number updates.
#define MINIMUM_FLASH_UPDATE_RATE_MINUTES		30		// The minimum period at which we are allowed to write to flash.
#define MAXIMUM_FLASH_UPDATE_RATE_MINUTES		180		// The maximum period allowed between flash updates.
#define TICKS_IN_A_MINUTE						60000		
#define FLASH_PAGE_SIZE							128		// Flash page size in words (32 bits).
#define NUMBER_OF_FIELDS_USED_BY_PROCESSOR		5		// Fields in the user page used by the processor for fuses.
#define NUMBER_OF_DEFAULT_COPIES				2		// Number of copies of the default data we are using.
#define FLASH_ACCESS_TICKS_TO_WAIT				5		// Number of ticks to wait for access to writing to flash. */

typedef struct _DEFAULT_VALUES_STRUCTURE
{
	uint32_t fieldUsedInternallyByTheProcessor[NUMBER_OF_FIELDS_USED_BY_PROCESSOR];
	uint32_t rfMode;
	uint32_t dataMode;
	uint32_t eclipseThresholdValue;
	uint32_t autonomousModeDefault;
	uint32_t sequenceNumber;
	uint32_t flashUpdateRate;
	uint32_t dataValid;
	uint32_t pageFiller[FLASH_PAGE_SIZE - 12];	// Make sure the next variable is not in the same flash page.
	uint32_t dataInvalid;
	uint32_t pageFiller1[FLASH_PAGE_SIZE - 1];	// Make sure this array takes up the whole flash page.
}DEFAULT_VALUES_STRUCTURE;

/* Default value update time memory */
void * defaultValueTimerID = 0;
StaticTimer_t defaultValueTimerBuffer;
volatile uint8_t defaultValuesUpdate = 0;					//	Indicate that we should update the default values.
static SemaphoreHandle_t flashAccessSemaphore;

static void UpdateDefaultValuesCallBack(xTimerHandle z_timer);
static void UpdateDefaultValuesInFlash(void);

static xTimerHandle updateDefaultsTimer = 0;

__attribute__((__section__(".flash_nvram")))
static DEFAULT_VALUES_STRUCTURE payloadDataStore[NUMBER_OF_DEFAULT_COPIES];

void InitialiseDefaultValues(SemaphoreHandle_t z_flashSemaphore)
{	
	uint32_t flashUpdateRateInTicks = 0;
	uint32_t memoryRegion = 0;
	uint32_t flashUpdateRate = MINIMUM_FLASH_UPDATE_RATE_MINUTES;

	/* Store the semaphore used for accessing flash. */
	flashAccessSemaphore = z_flashSemaphore;	
	defaultValuesUpdate = 0;

	/* If we dont have valid data then we need to allow the CAN OD to boot up using its own initial hard-coded default values and save them to flash. */
	if(payloadDataStore[0].dataValid <= payloadDataStore[0].dataInvalid && payloadDataStore[1].dataValid <= payloadDataStore[1].dataInvalid)
	{
		/* Scheduler hasnt started yet so we can do this guilt free without locking */
		UpdateDefaultValuesInFlash();
	}
	else
	{
		uint32_t sequenceNumber = 0;
		
		/* We have valid data so we should read out of flash and write into the CAN OD */
		if(payloadDataStore[0].dataValid > payloadDataStore[0].dataInvalid)
		{
			memoryRegion = 0;
		}
		else if(payloadDataStore[1].dataValid > payloadDataStore[1].dataInvalid)
		{
			memoryRegion = 1;
		}
		else
		{
			/* An error has occurred - we already check for this above anyway! */
		}

		/* Limit the maximum flash update rate. */
		if(payloadDataStore[memoryRegion].flashUpdateRate < MINIMUM_FLASH_UPDATE_RATE_MINUTES)
		{
			flashUpdateRate = MINIMUM_FLASH_UPDATE_RATE_MINUTES;
		}
		else if(payloadDataStore[memoryRegion].flashUpdateRate > MAXIMUM_FLASH_UPDATE_RATE_MINUTES)
		{
			flashUpdateRate = MAXIMUM_FLASH_UPDATE_RATE_MINUTES;
		}
		else
		{
			flashUpdateRate = payloadDataStore[memoryRegion].flashUpdateRate;
		}

		/* Calculate and perform the increment of how much we need to increment our sequence number by on a reset. */
		sequenceNumber = payloadDataStore[memoryRegion].sequenceNumber + (payloadDataStore[memoryRegion].flashUpdateRate / SEQUENCE_NUMBER_UPDATE_PERIOD_MINUTES);

		/* Update the CAN OD with the stored default values. Don't use queue as we haven't started the scheduler yet! */
		SetCANObjectDataRaw(AMS_OBC_RFMODE_DEF, payloadDataStore[memoryRegion].rfMode);							// RF Mode
		SetCANObjectDataRaw(AMS_OBC_DATAMODE_DEF, payloadDataStore[memoryRegion].dataMode);						// Data mode
		SetCANObjectDataRaw(AMS_OBC_ECLIPSE_THRESH, payloadDataStore[memoryRegion].eclipseThresholdValue);		// Eclipse Threshold value
		SetCANObjectDataRaw(AMS_OBC_ECLIPSE_MODE_DEF, payloadDataStore[memoryRegion].autonomousModeDefault);	// Autonomous mode default
		SetCANObjectDataRaw(AMS_OBC_SEQNUM, sequenceNumber);													// Sequence number
		SetCANObjectDataRaw(AMS_OBC_FLASH_RATE, flashUpdateRate);												// Flash update rate
	}

	/* Limit the maximum flash update rate. */
	if(payloadDataStore[memoryRegion].flashUpdateRate < MINIMUM_FLASH_UPDATE_RATE_MINUTES)
	{
		flashUpdateRate = MINIMUM_FLASH_UPDATE_RATE_MINUTES;
	}

	/* Convert from minutes to ticks to pass to timer. */
	flashUpdateRateInTicks =  flashUpdateRate * TICKS_IN_A_MINUTE;

	/* Create a timer for updating flash in the future. */
	updateDefaultsTimer = xTimerCreateStatic("DefaultValues", flashUpdateRateInTicks, AUTO_RELOAD, &defaultValueTimerID, &UpdateDefaultValuesCallBack , &defaultValueTimerBuffer);
	xTimerStart(updateDefaultsTimer, 0);
}

static void UpdateDefaultValuesInFlash(void)
{
	uint8_t i = 0;
	for(i = 0; i < NUMBER_OF_DEFAULT_COPIES; i++)
	{
		uint32_t data = 0;
		ClearPageBuffer();
		GetCANObjectDataRaw(AMS_OBC_RFMODE_DEF, &data);			// RF Mode
		payloadDataStore[i].rfMode = data;
		GetCANObjectDataRaw(AMS_OBC_DATAMODE_DEF, &data);		// Data mode
		payloadDataStore[i].dataMode = data;
		GetCANObjectDataRaw(AMS_OBC_ECLIPSE_THRESH, &data);		// Eclipse Threshold value
		payloadDataStore[i].eclipseThresholdValue = data;
		GetCANObjectDataRaw(AMS_OBC_ECLIPSE_MODE_DEF, &data);	// Autonomous mode default
		payloadDataStore[i].autonomousModeDefault = data;
		GetCANObjectDataRaw(AMS_OBC_SEQNUM, &data);				// Sequence number
		payloadDataStore[i].sequenceNumber = data;
		GetCANObjectDataRaw(AMS_OBC_FLASH_RATE, &data);			// Flash update rate
		payloadDataStore[i].flashUpdateRate = data;
		/* Mark the section as valid once we have written to it */

		if(flashAccessSemaphore != 0)
		{
			if(xSemaphoreTakeRecursive(flashAccessSemaphore, FLASH_ACCESS_TICKS_TO_WAIT) != 0)
			{		
				payloadDataStore[i].dataValid = payloadDataStore[i].dataValid + 1;
				ClearPage();
				WriteToFlash();

				ClearPageBuffer();
				payloadDataStore[i].dataInvalid = payloadDataStore[i].dataValid - 1;
				ClearPage();
				WriteToFlash();
				xSemaphoreGiveRecursive(flashAccessSemaphore);
			}
		}
	}
}

void UpdateDefaultValues(void)
{
	if(defaultValuesUpdate != 0)
	{
		UpdateDefaultValuesInFlash();
		defaultValuesUpdate = 0;
	}
}

static void UpdateDefaultValuesCallBack(xTimerHandle z_timer)
{
	defaultValuesUpdate = 1;
}