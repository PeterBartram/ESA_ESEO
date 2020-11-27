
#include "CANObjectDictionary.h"
#include "CANODInterface.h"
#include "CAN_PDO.h"
#include "ExtendedObjectDictionaryFields.h"
#include "FreeRTOS.h"
#include "semphr.h"
//#include "Commanding.h"


/* Union used for reversing the endianism of 32 bit values. */
typedef union
{
	uint32_t data;
	struct
	{
		uint8_t data0;
		uint8_t data1;
		uint8_t data2;
		uint8_t data3;
	};
}ENDIANISM_FLIP;

/** Enumeration for stating the size of elements in the CAN OD */
typedef enum _OBJECT_DICTIONARY_ENTRY_BIT_SIZE
{
	BIT_SIZE_1 = 0,
	BIT_SIZE_2,
	BIT_SIZE_3,
	BIT_SIZE_8,
	BIT_SIZE_16,
	BIT_SIZE_24,
	BIT_SIZE_32
}OBJECT_DICTIONARY_ENTRY_BIT_SIZE;

#define DIFFERENT_ELEMENT_SIZES 7 //Number of different element sizes in the OBJECT_DICTIONARY_ENTRY_BIT_SIZE enumeration.

/** Lookup table used to mask the output of the elements in the CAN OD */
uint32_t objectDictionaryElementMaskTable[DIFFERENT_ELEMENT_SIZES] = 
{
	0x00000001,
	0x00000003,
	0x00000007,
	0x000000FF,
	0x0000FFFF,
	0x00FFFFFF,
	0xFFFFFFFF
};

/** Enumeration for the possible access rights for an object dictionary entry. */
typedef enum
{
	NONE,			/**< An object dictionary entry configured with this access right is not in use. */
	GET,			/**< Read only access */
	SET,			/**< Write only access */
	GET_AND_SET,	/**< Read and write access */
	SETT			/**< Broadcast set access only. */
}OBJECT_DICTIONARY_ACCESS_RIGHT;

typedef enum
{
	ENTRY_UNTOUCHED	= 0,		/**< This entry has not been touched since it was last serviced. */
	ENTRY_CHANGED,				/**< An object dictionary entry has changed. */
	ENTRY_WRITTEN				/**< An object dictionary entry has been written to */	
}DICTIONARY_ENTRY_STATE;

/** This structure represents a single object dictionary input, all ROM
	data has been moved into OBJECT_DICTIONARY_CONFIGURATION_ENTRY to allow 
	for it to be placed into flash to better radiation harden the design. */
typedef struct
{
	uint32_t data[3];						/**< ObjectDictionary data storage */
	DICTIONARY_ENTRY_STATE changed;		/**< State of the dictionary entry in terms of changes in data. */
}OBJECT_DICTIONARY_ENTRY;

/** This structure holds configuration data directly related to elements
	within OBJECT_DICTIONARY_ENTRY and there must be a 1 to 1 relationship
	between them such that each entry has both runtime and config data. */
typedef struct
{
	OBJECT_DICTIONARY_ACCESS_RIGHT acesssRights;											/**< Object access rights. */
	OBJECT_DICTIONARY_ENTRY_BIT_SIZE dataSize;												/**< Enumeration for total bits used for this element. */
	CAN_OPEN_ERROR (*executeActionGet)(uint8_t z_id, uint32_t * z_data);					/**< Function to be called when this object has been read from. */
	CAN_OPEN_ERROR (*executeAction)(uint8_t z_id, uint32_t z_data, uint8_t z_changed);		/**< Function to be called when this object has been written to. */
}OBJECT_DICTIONARY_CONFIGURATION_ENTRY;

/* Permanent defines - don't change these! */
#define	OBJECT_DICTIONARY_ENTRIES	0x63			/**< The number of entries in our object dictionary */
#define CAN_OD_ACCESS_TIME_TO_WAIT	5				/**< Number of ticks to wait on a semaphore */

static uint32_t ChangeEndianism(uint32_t z_input);
static uint32_t CompareAndCorrectItemElement(OBJECT_DICTIONARY_ENTRY * z_entry);

#define RF_MODE_DEFAULT							1	//	Low power telemetry
#define USE_3_PANELS_FOR_ECLIPSE_DETECT			3
#define CTCSS_DETECT_ON							1		// Look for a CTCSS tone by default
#define SAFE_MODE_ON_DEFAULT					1		// Safe mode is activated by default
#define HEARTBEAT_WATCHDOG_TIMER_DEFAULT		10		// 10 second heartbeat timeout
#define LAST_COMMAND_WATCHDOG_TIMER				120		// 120 hour command reciept watchdog
#define OVER_TEMPERATURE_PROTECTION_DEFAULT		1		// Over temperature detect enabled by default
#define FLASH_UPDATE_RATE_DEFAULT				30		// 30 minute default updated rate (this is limited elsewhere anyway)
#define SATELLITE_ID_NUMBER						102

/*	This is the object dictionary itself */
static OBJECT_DICTIONARY_ENTRY objectDictionary[OBJECT_DICTIONARY_ENTRIES]	=
{
	{SATELLITE_ID_NUMBER,SATELLITE_ID_NUMBER,SATELLITE_ID_NUMBER,		ENTRY_UNTOUCHED},				//0x0
	{0,0,0,		ENTRY_UNTOUCHED},				//0x1
	{0,0,0,		ENTRY_UNTOUCHED},				//0x2
	{0,0,0,		ENTRY_UNTOUCHED},				//0x3
	{0,0,0,		ENTRY_UNTOUCHED},				//0x4
	{0,0,0,		ENTRY_UNTOUCHED},				//0x5
	{0,0,0,		ENTRY_UNTOUCHED},				//0x6
	{0,0,0,		ENTRY_UNTOUCHED},				//0x7
	{0,0,0,		ENTRY_UNTOUCHED},				//0x8
	{0,0,0,		ENTRY_UNTOUCHED},				//0x9
	{0,0,0,		ENTRY_UNTOUCHED},				//0xA
	{0,0,0,		ENTRY_UNTOUCHED},				//0xB
	{0,0,0,		ENTRY_UNTOUCHED},				//0xC
	{0,0,0,		ENTRY_UNTOUCHED},				//0xD
	{0,0,0,		ENTRY_UNTOUCHED},				//0xE
	{0,0,0,		ENTRY_UNTOUCHED},				//0xF
	{RF_MODE_DEFAULT, RF_MODE_DEFAULT, RF_MODE_DEFAULT,		ENTRY_UNTOUCHED},																	//0x10
	{0,0,0,		ENTRY_UNTOUCHED},				//0x11
	{0,0,0,		ENTRY_UNTOUCHED},				//0x12
	{0,0,0,		ENTRY_UNTOUCHED},				//0x13
	{0,0,0,		ENTRY_UNTOUCHED},				//0x14
	{0,0,0,		ENTRY_UNTOUCHED},				//0x15
	{0,0,0,		ENTRY_UNTOUCHED},				//0x16
	{0,0,0,		ENTRY_UNTOUCHED},				//0x17
	{0,0,0,		ENTRY_UNTOUCHED},				//0x18
	{0,0,0,		ENTRY_UNTOUCHED},				//0x19
	{0,0,0,		ENTRY_UNTOUCHED},				//0x1A
	{USE_3_PANELS_FOR_ECLIPSE_DETECT, USE_3_PANELS_FOR_ECLIPSE_DETECT, USE_3_PANELS_FOR_ECLIPSE_DETECT,		ENTRY_UNTOUCHED},					//0x1B
	{0,0,0,		ENTRY_UNTOUCHED},				//0x1C
	{0,0,0,		ENTRY_UNTOUCHED},				//0x1D
	{CTCSS_DETECT_ON, CTCSS_DETECT_ON, CTCSS_DETECT_ON,		ENTRY_UNTOUCHED},																	//0x1E
	{SAFE_MODE_ON_DEFAULT, SAFE_MODE_ON_DEFAULT, SAFE_MODE_ON_DEFAULT,		ENTRY_UNTOUCHED},													//0x1F
	{0,0,0,		ENTRY_UNTOUCHED},				//0x20
	{0,0,0,		ENTRY_UNTOUCHED},				//0x21
	{0,0,0,		ENTRY_UNTOUCHED},				//0x22
	{0,0,0,		ENTRY_UNTOUCHED},				//0x23
	{0,0,0,		ENTRY_UNTOUCHED},				//0x24
	{HEARTBEAT_WATCHDOG_TIMER_DEFAULT, HEARTBEAT_WATCHDOG_TIMER_DEFAULT, HEARTBEAT_WATCHDOG_TIMER_DEFAULT,		ENTRY_UNTOUCHED},				//0x25
	{LAST_COMMAND_WATCHDOG_TIMER, LAST_COMMAND_WATCHDOG_TIMER, LAST_COMMAND_WATCHDOG_TIMER,		ENTRY_UNTOUCHED},								//0x26
	{OVER_TEMPERATURE_PROTECTION_DEFAULT, OVER_TEMPERATURE_PROTECTION_DEFAULT, OVER_TEMPERATURE_PROTECTION_DEFAULT,		ENTRY_UNTOUCHED},		//0x27
	{0,0,0,		ENTRY_UNTOUCHED},				//0x28
	{0,0,0,		ENTRY_UNTOUCHED},				//0x29
	{0,0,0,		ENTRY_UNTOUCHED},				//0x2A
	{0,0,0,		ENTRY_UNTOUCHED},				//0x2B
	{0,0,0,		ENTRY_UNTOUCHED},				//0x2C
	{0,0,0,		ENTRY_UNTOUCHED},				//0x2D
	{0,0,0,		ENTRY_UNTOUCHED},				//0x2E
	{0,0,0,		ENTRY_UNTOUCHED},				//0x2F
	{FLASH_UPDATE_RATE_DEFAULT, FLASH_UPDATE_RATE_DEFAULT, FLASH_UPDATE_RATE_DEFAULT,		ENTRY_UNTOUCHED},									//0x30
	{0,0,0,		ENTRY_UNTOUCHED},				//0x31
	{0,0,0,		ENTRY_UNTOUCHED},				//0x32
	{0,0,0,		ENTRY_UNTOUCHED},				//0x33
	{0,0,0,		ENTRY_UNTOUCHED},				//0x34
	{0,0,0,		ENTRY_UNTOUCHED},				//0x35
	{0,0,0,		ENTRY_UNTOUCHED},				//0x36
	{0,0,0,		ENTRY_UNTOUCHED},				//0x37
	{0,0,0,		ENTRY_UNTOUCHED},				//0x38
	{0,0,0,		ENTRY_UNTOUCHED},				//0x39
	{0,0,0,		ENTRY_UNTOUCHED},				//0x3A
	{0,0,0,		ENTRY_UNTOUCHED},				//0x3B
	{0,0,0,		ENTRY_UNTOUCHED},				//0x3C
	{0,0,0,		ENTRY_UNTOUCHED},				//0x3D
	{0,0,0,		ENTRY_UNTOUCHED},				//0x3E
	{0,0,0,		ENTRY_UNTOUCHED},				//0x3F
	{0,0,0,		ENTRY_UNTOUCHED},				//0x40
	{0,0,0,		ENTRY_UNTOUCHED},				//0x41
	{0,0,0,		ENTRY_UNTOUCHED},				//0x42
	{0,0,0,		ENTRY_UNTOUCHED},				//0x43
	{0,0,0,		ENTRY_UNTOUCHED},				//0x44
	{0,0,0,		ENTRY_UNTOUCHED},				//0x45
	{0,0,0,		ENTRY_UNTOUCHED},				//0x46
	{0,0,0,		ENTRY_UNTOUCHED},				//0x47
	{0,0,0,		ENTRY_UNTOUCHED},				//0x48
	{0,0,0,		ENTRY_UNTOUCHED},				//0x49
	{0,0,0,		ENTRY_UNTOUCHED},				//0x4A
	{0,0,0,		ENTRY_UNTOUCHED},				//0x4B
	{0,0,0,		ENTRY_UNTOUCHED},				//0x4C
	{0,0,0,		ENTRY_UNTOUCHED},				//0x4D
	{0,0,0,		ENTRY_UNTOUCHED},				//0x4E
	{0,0,0,		ENTRY_UNTOUCHED},				//0x4F
	{0,0,0,		ENTRY_UNTOUCHED},				//0x50
	{0,0,0,		ENTRY_UNTOUCHED},				//0x51
	{0,0,0,		ENTRY_UNTOUCHED},				//0x52
	{0,0,0,		ENTRY_UNTOUCHED},				//0x53
	{0,0,0,		ENTRY_UNTOUCHED},				//0x54
	{0,0,0,		ENTRY_UNTOUCHED},				//0x55
	{0,0,0,		ENTRY_UNTOUCHED},				//0x56
	{0,0,0,		ENTRY_UNTOUCHED},				//0x57
	{0,0,0,		ENTRY_UNTOUCHED},				//0x58
	{0,0,0,		ENTRY_UNTOUCHED},				//0x59
	{0,0,0,		ENTRY_UNTOUCHED},				//0x5A
	{0,0,0,		ENTRY_UNTOUCHED},				//0x5B
	{0,0,0,		ENTRY_UNTOUCHED},				//0x5C
	{0,0,0,		ENTRY_UNTOUCHED},				//0x5D
	{0,0,0,		ENTRY_UNTOUCHED},				//0x5E
	{0,0,0,		ENTRY_UNTOUCHED},				//0x5F
	{0,0,0,		ENTRY_UNTOUCHED},				//0x60
	{0,0,0,		ENTRY_UNTOUCHED},				//0x61
	{0,0,0,		ENTRY_UNTOUCHED}				//0x62
};

/* This is the configuration data for the objectdictionary	- 
	this is ultimately going to be placed into flash */
static const OBJECT_DICTIONARY_CONFIGURATION_ENTRY objectDictionaryConfiguration[OBJECT_DICTIONARY_ENTRIES]	=	
{
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x0
	{SETT,			BIT_SIZE_24,		NULL,					NULL},											 		//0x1
	{SETT,			BIT_SIZE_32,		NULL,					NULL},											 		//0x2
	{GET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x3
	{GET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x4
	{GET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x5
	{GET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x6
	{GET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x7
	{GET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x8
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x9
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0xA
	{GET,			BIT_SIZE_32,		NULL,					NULL},											 		//0xB
	{SET,			BIT_SIZE_1,			NULL,					NULL},											 		//0xC
	{GET,			BIT_SIZE_1,			NULL,					NULL},											 		//0xD
	{GET,			BIT_SIZE_1,			NULL,					NULL},											 		//0xE
	{GET,			BIT_SIZE_1,			NULL,					NULL},											 		//0xF
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x10
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x11
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x12
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x13
	{GET_AND_SET,	BIT_SIZE_32,		NULL,					ReceivedPayloadDataTransferProtocolRequest},	 		//0x14
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x15
	{GET_AND_SET,	BIT_SIZE_32,		GetHolemapField,		SetHolemapField},								 		//0x16	Holemap Register
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					ChangeFitterMessageSlot},						 		//0x17	Fitter message slot register
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x18
	{GET_AND_SET,	BIT_SIZE_32,		GetFitterMessageField,	SetFitterMessageField},							 		//0x19	Fitter message field.
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x1A
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x1B
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x1C
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x1D
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x1E
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x1F
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x20
	{GET_AND_SET,	BIT_SIZE_32,		NULL,					NULL},											 		//0x21
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x22
	{GET_AND_SET,	BIT_SIZE_24,		NULL,					NULL},											 		//0x23
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x24
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x25
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x26
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x27
	{GET_AND_SET,	BIT_SIZE_32,		NULL,					NULL},											 		//0x28
	{GET_AND_SET,	BIT_SIZE_32,		NULL,					NULL},											 		//0x29
	{GET_AND_SET,	BIT_SIZE_32,		NULL,					NULL},											 		//0x2A
	{GET_AND_SET,	BIT_SIZE_32,		NULL,					NULL},											 		//0x2B
	{GET_AND_SET,	BIT_SIZE_8,			NULL,					NULL},											 		//0x2C
	{GET_AND_SET,	BIT_SIZE_32,		NULL,					NULL},											 		//0x2D
	{GET_AND_SET,	BIT_SIZE_32,		NULL,					NULL},											 		//0x2E
	{GET_AND_SET,	BIT_SIZE_32,		NULL,					NULL},											 		//0x2F
	{GET_AND_SET,	BIT_SIZE_32,		NULL,					NULL},											 		//0x30
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x31
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x32
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x33
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x34
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x35
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x36
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x37
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x38
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x39
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x3A
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x3B
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x3C
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x3D
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x3E
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x3F
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x40
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x41
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x42
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x43
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x44
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x45
	{GET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x46
	{SET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x47
	{SET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x48
	{SET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x49
	{SET,			BIT_SIZE_8,			NULL,					NULL},											 		//0x4A
	{SET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x4B										
	{SET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x4C
	{SET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x4D
	{SET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x4E
	{SET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x4F
	{SET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x50
	{SET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x51
	{SET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x52
	{SET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x53
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x54
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x55
	{SET,			BIT_SIZE_32,		NULL,					NULL},											 		//0x56
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x57
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x58
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x59
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x5A
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x5B
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x5C
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x5D
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x5E
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x5F
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x60
	{SET,			BIT_SIZE_16,		NULL,					NULL},											 		//0x61
	{SET,			BIT_SIZE_16,		NULL,					NULL}											 		//0x62
};
																												 
/* CAN OD access semaphore */																					 
SemaphoreHandle_t canObjectDictionarySemaphore;																	 
StaticSemaphore_t canObjectDictionarySemaphoreStorage;

/**
*	Initialise the CAN OD - configure default values and also
*	the mutex to ensure concurrency.
*/
void InitialiseCanObjectDictionary(void)
{
	/* Create our semaphore to protect the OD. */
	 canObjectDictionarySemaphore = xSemaphoreCreateRecursiveMutexStatic(&canObjectDictionarySemaphoreStorage);
	 InitialiseExtendedObjectFields(canObjectDictionarySemaphore);
}

/**
*	Set a 32bit CAN OD value. This is a RAW SET command and does not 
*	follow the usual endinism-swapping / get/set procedures followed 
*	when setting an object via the network/uplink.
*	As such it should not be used to get/set any data from/to the
*    network
*	\param[in]	z_index: the OD index to write to.
*	\param[in]	z_data: the data to write to the index.
*	\return NO_ERROR_OBJECT_DICTIONARY if successful, otherwise an error code will be returned.
*/
OBJECT_DICTIONARY_ERROR	SetCANObjectDataRaw(uint32_t z_index, uint32_t z_data)
{
	OBJECT_DICTIONARY_ERROR retVal = NO_ERROR_OBJECT_DICTIONARY;

	if(z_index < (sizeof(objectDictionary)/sizeof(objectDictionary[0])))
	{
		if(canObjectDictionarySemaphore != 0)
		{
			if(xSemaphoreTakeRecursive(canObjectDictionarySemaphore, CAN_OD_ACCESS_TIME_TO_WAIT))
			{
				/* Triplicate the data in the OD. */
				objectDictionary[z_index].data[0] = z_data;
				objectDictionary[z_index].data[1] = z_data;
				objectDictionary[z_index].data[2] = z_data;

				/* Return our semaphore */
				xSemaphoreGiveRecursive(canObjectDictionarySemaphore);
			}
			else
			{
				retVal = SEMAPHORE_TIME_OUT;
			}
		}
		else
		{
			retVal = SEMAPHORE_INVALID;
		}			
	}

	return retVal;
}

/**
*	Get a 32bit CAN OD value. This is a RAW SET command and does not 
*	follow the usual endinism-swapping / get/set proceedures followed 
*	when setting an object via the network/uplink.
*	As such it should not be used to get/set any data from/to the
*    network
*	\param[in]	z_index: the OD index to write to.
*	\param[in]	z_data: the data to write to the index.
*	\return NO_ERROR_OBJECT_DICTIONARY if successful, otherwise an error code will be returned.
*/
OBJECT_DICTIONARY_ERROR	GetCANObjectDataRaw(uint32_t z_index, uint32_t * z_data)
{
	OBJECT_DICTIONARY_ERROR retVal = NO_ERROR_OBJECT_DICTIONARY;

	if(z_data != 0)
	{
		if(z_index < (sizeof(objectDictionary)/sizeof(objectDictionary[0])))
		{
			if(canObjectDictionarySemaphore != 0)
			{
				if(xSemaphoreTakeRecursive(canObjectDictionarySemaphore, CAN_OD_ACCESS_TIME_TO_WAIT))
				{
					/* Correct for any bit errors before we use the entry */
					CompareAndCorrectItemElement(&objectDictionary[z_index]);

					/* Mask the output value with the data size mask */
					*z_data = objectDictionary[z_index].data[0] & objectDictionaryElementMaskTable[objectDictionaryConfiguration[z_index].dataSize];
					/* Return our semaphore */
					xSemaphoreGiveRecursive(canObjectDictionarySemaphore);
				}
				else
				{
					retVal = SEMAPHORE_TIME_OUT;
				}
			}
			else
			{
				retVal = SEMAPHORE_INVALID;
			}			
		}

		/* If we have had an error then return zero for our data. */
		if(retVal != NO_ERROR_OBJECT_DICTIONARY)
		{
			*z_data = 0;
		}
	}

	return retVal;
}

/**
*	Interface function - Set a 32bit CAN object value based upon a SET command received via CAN,
*	this method should not be used for setting outside of the CAN communications functionality.
*	Instead, the alternative methods for writing to a single variable should be used.
*	\param[in]	z_index: the OD index to write to.
*	\param[in]	z_data: the data to write to the index.
*	\return NO_ERROR_OBJECT_DICTIONARY if successful, otherwise an error code will be returned.
*/
OBJECT_DICTIONARY_ERROR	SetCANObjectData(uint32_t z_index, uint32_t z_data)
{
	OBJECT_DICTIONARY_ERROR retVal = NO_ERROR_OBJECT_DICTIONARY;
	
	/* Check that this entry is actually in use. */
	if(objectDictionaryConfiguration[z_index].acesssRights != NONE)
	{
		/* Check this elements read / write access */
		if(objectDictionaryConfiguration[z_index].acesssRights == SET || objectDictionaryConfiguration[z_index].acesssRights == GET_AND_SET)
		{
			if(canObjectDictionarySemaphore != 0)
			{
				if(xSemaphoreTakeRecursive(canObjectDictionarySemaphore, CAN_OD_ACCESS_TIME_TO_WAIT))
				{
					/* Change to big endianism to match our dictionary. */
					z_data = ChangeEndianism(z_data);

					/* Correct for any bit errors before we use the entry */
					CompareAndCorrectItemElement(&objectDictionary[z_index]);

					/* Check if we have new data being written. */
					if(objectDictionary[z_index].data[0] != z_data)
					{
						objectDictionary[z_index].changed = ENTRY_CHANGED;
					}
					else
					{
						/* Don't overwrite entry changed with entry written */
						if(objectDictionary[z_index].changed != ENTRY_CHANGED)
						{
							objectDictionary[z_index].changed = ENTRY_WRITTEN;
						}
					}
					/* Update the dictionary entry. */
					objectDictionary[z_index].data[0] = z_data;
					objectDictionary[z_index].data[1] = z_data;
					objectDictionary[z_index].data[2] = z_data;
			
					/* Is there any action to be performed due to this update? */
					PerformEventActions(z_index);

					/* Return our semaphore */
					xSemaphoreGiveRecursive(canObjectDictionarySemaphore);
				}
				else
				{
					retVal = SEMAPHORE_TIME_OUT;
				}
			}
			else
			{
				retVal = SEMAPHORE_INVALID;
			}
		}
		else
		{
			retVal = WRONG_ACCESS_RIGHTS;
		}
	}
	else
	{
		retVal = INVALID_INDEX_OBJECT_DICTIONARY;
	}

	return retVal;
}

/**
*	Interface function - Get a 32bit CAN object value based upon a GET command recevied via CAN,
*	this method should not be used for getting outside of the CAN communications functionality.
*	Instead, the alternative methods for writing to a single variable should be used.
*	\param[in]	z_index: the OD index to write to.
*	\param[out]	z_data: the data in the index read from.
*	\return NO_ERROR_OBJECT_DICTIONARY if successful, otherwise an error code will be returned.
*/
OBJECT_DICTIONARY_ERROR	GetCANObjectData(uint32_t z_index, uint32_t * z_data)
{
	OBJECT_DICTIONARY_ERROR retVal = NO_ERROR_OBJECT_DICTIONARY;
	
	if(z_data != 0)
	{
		/* Check that this entry is actually in use. */
		if(objectDictionaryConfiguration[z_index].acesssRights != NONE)
		{
			/* Check this elements read / write access */
			if(objectDictionaryConfiguration[z_index].acesssRights == GET || objectDictionaryConfiguration[z_index].acesssRights == GET_AND_SET)
			{
				if(canObjectDictionarySemaphore != 0)
				{
					if(xSemaphoreTakeRecursive(canObjectDictionarySemaphore, CAN_OD_ACCESS_TIME_TO_WAIT))
					{
						/* Correct for any bit errors before we use the entry */
						CompareAndCorrectItemElement(&objectDictionary[z_index]);

						/* We have an action to perform */
						if(objectDictionaryConfiguration[z_index].executeActionGet != 0)
						{
							/* Change the returned value based on the get command. */
							objectDictionaryConfiguration[z_index].executeActionGet(z_index, &objectDictionary[z_index].data[0]);
						}
						/* Return the object dictionary data */
						*z_data = ChangeEndianism(objectDictionary[z_index].data[0]);

						/* Return our semaphore */
						xSemaphoreGiveRecursive(canObjectDictionarySemaphore);
					}
					else
					{
						retVal = SEMAPHORE_TIME_OUT;
					}
				}
				else
				{
					retVal = SEMAPHORE_INVALID;
				}
			}
			else
			{
				retVal = WRONG_ACCESS_RIGHTS;
			}		
		}
		else
		{
			retVal = INVALID_INDEX_OBJECT_DICTIONARY;
		}

		/* Return zero data in the case of an error */
		if(retVal != NO_ERROR_OBJECT_DICTIONARY)
		{
			*z_data	=	0;
		}
	}
	else
	{
		retVal = NULL_POINTER_OBJECT_DICTIONARY;
	}

	return retVal;
}

/**
*	Interface function - Set the time based upon a broadcast 32 bit CAN value sent
*						 over the SETT RDPO;
*	\param[in]	z_index: the OD index to write to.
*	\param[out]	z_data: the data in the index read from.
*/
void SettCANObjectData(uint32_t z_index, uint32_t z_data)
{
	/* Don't update without correct access rights. */
	if(objectDictionaryConfiguration[z_index].acesssRights == SETT)
	{
		z_data = ChangeEndianism(z_data);

		/* Correct for any bit errors before we use the entry */
		CompareAndCorrectItemElement(&objectDictionary[z_index]);

		/* Check if we have new data being written. */
		if(objectDictionary[z_index].data[0] != z_data)
		{
			objectDictionary[z_index].changed = ENTRY_CHANGED;
		}
		else
		{
			/* Don't overwrite entry changed with entry written */
			if(objectDictionary[z_index].changed != ENTRY_CHANGED)
			{
				objectDictionary[z_index].changed = ENTRY_WRITTEN;
			}
		}

		/* Update the dictionary entry. */
		objectDictionary[z_index].data[0] = z_data;
		objectDictionary[z_index].data[1] = z_data;
		objectDictionary[z_index].data[2] = z_data;
		
		/* Is there any action to be performed due to this update? */
		PerformEventActions(z_index);		
	}
}

static uint32_t housekeepingData[5];
/**
*	Interface function - Returns a pointer to our housekeeping data and also the size of it.
*	\param[out]	z_data: housekeeping data string.
*	\return number of elements in the housekeeping data.
*/
uint8_t GetHousekeepingData(uint8_t ** z_data)
{
	uint32_t data = 0;
	uint8_t retVal = 0;

	if(z_data != 0)
	{
		/* Collect the required housekeeping data and place into memory */
		GetCANObjectData(AMS_OBC_P_UP, &data);
		housekeepingData[0] = data;
		GetCANObjectData(AMS_OBC_P_UP_DROPPED, &data);
		housekeepingData[1] = data;
		GetCANObjectData(AMS_OBC_MEM_STAT_1, &data);
		housekeepingData[2] = data;
		GetCANObjectData(AMS_OBC_MEM_STAT_2, &data);
		housekeepingData[3] = data;
		/* All 8 bit values so we pack them into words. */
		GetCANObjectData(AMS_OBC_LC, &data);
		housekeepingData[4] = data;
		GetCANObjectData(AMS_EPS_DCDC_T, &data);
		housekeepingData[4] |= (data >> 8);
		GetCANObjectData(AMS_VHF_FM_PA_T, &data);
		housekeepingData[4] |= (data >> 16);
		GetCANObjectData(AMS_VHF_BPSK_PA_T, &data);
		housekeepingData[4] |= (data >> 24);
	
		*z_data = (uint8_t *)housekeepingData;
		retVal = 20;
	}
	return retVal;
}

/**
*	Interface function - Go through the list of object dictionary entries
*	and perform any actions that are necessary.
*	@todo this table of function pointers is very wasteful - replace it with a case statement?
*/
void PerformEventActions(uint8_t z_index)
{
	/* If memory location is in use. */
	if(objectDictionaryConfiguration[z_index].acesssRights != NONE)
	{
		/* If this entry has been updated. */
		if(objectDictionary[z_index].changed != ENTRY_UNTOUCHED)
		{
			/* If we have an action to perform. */
			if(objectDictionaryConfiguration[z_index].executeAction != 0)
			{
				uint8_t changed;

				/* Determine whether we have been changed or not. */
				changed = objectDictionary[z_index].changed == ENTRY_CHANGED ? 1 : 0;

				/* Execute the action for this dictionary entry. */
				if(objectDictionaryConfiguration[z_index].executeAction(z_index, objectDictionary[z_index].data[0], changed) == NO_ERROR_OBJECT_DICTIONARY)
				{
					/* We have executed this action now so we can reset the state for the next update. */
					objectDictionary[z_index].changed = ENTRY_UNTOUCHED;
				}
			}
		}
	}
}

/* This function looks at the CAN ID passed in and decides if it is a fitter, holemap or regular message.
   Ideally I would have had a field in the CAN OD so that we could have done this completely generically
   but I couldnt justify using 1.6kB of memory for three special cases so this is a compromise. */
OD_DATA_TYPE GetFieldType(uint8_t z_id)
{
	OD_DATA_TYPE retVal = NORMAL_FIELD;

	if(z_id == AMS_OBC_HM_REGISTER)
	{
		retVal = HOLEMAP_FIELD;
	}
	else if(z_id == AMS_OBC_FM_REGISTER)
	{
		retVal = FITTER_FIELD;
	}
	else if(z_id == AMS_OBC_DEBUG_REGISTER)
	{
		retVal = DEBUG_FIELD;
	}
	return retVal;
}

/**
*	Private function - detect and correct any bit errors in a CAN OD field.
*	\param[in]	z_entry - the CAN OD entry we are referencing.
*	\return The corrected value.
*/
static uint32_t CompareAndCorrectItemElement(OBJECT_DICTIONARY_ENTRY * z_entry)
{
	uint32_t retVal = 0;
	if(z_entry != 0)
	{
		if((z_entry->data[0] == z_entry->data[1]) && (z_entry->data[0] == z_entry->data[2]))
		{
			retVal = z_entry->data[0];
		}
		else
		{
			if(z_entry->data[0] == z_entry->data[1])
			{
				retVal = z_entry->data[0];				// Return element 0
				z_entry->data[2] = z_entry->data[0];	// Correct fault element
			}
			else if(z_entry->data[0] == z_entry->data[2])
			{
				retVal = z_entry->data[0];				// Return element 0
				z_entry->data[1] = z_entry->data[0];	// Correct fault element
			}
			else if(z_entry->data[1] == z_entry->data[2])
			{
				retVal = z_entry->data[1];				// Return element 0
				z_entry->data[0] = z_entry->data[1];	// Correct fault element
			}
			else
			{
				/* Unrecoverable error has occurred - trigger the watch dog and reset processor. */
			}
		}
	}
	return retVal;
}

/**
*	Private function - flip the endianism of a 32 bit number.
*	\param[in]	z_input - the 32bit value to reverse.
*	\return The endianism changed value.
*/
static uint32_t ChangeEndianism(uint32_t z_input)
{
		ENDIANISM_FLIP flip;
		uint8_t temp;
		flip.data = z_input;
		temp = flip.data0;
		flip.data0 = flip.data3;
		flip.data3 = temp;
		temp = flip.data1;
		flip.data1 = flip.data2;
		flip.data2 = temp;
		return flip.data;
}

