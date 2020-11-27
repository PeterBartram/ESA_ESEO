#ifndef CANOBJECTDICTIONARY_H_
#define CANOBJECTDICTIONARY_H_

#include "stdint.h"
#include "CANOpen.h"


/* Errors */
typedef enum _OBJECT_DICTIONARY_ERROR
{
	NO_ERROR_OBJECT_DICTIONARY,
	INVALID_INDEX_OBJECT_DICTIONARY,
	NULL_POINTER_OBJECT_DICTIONARY,
	WRONG_ACCESS_RIGHTS,
	SEMAPHORE_TIME_OUT,
	SEMAPHORE_INVALID
}OBJECT_DICTIONARY_ERROR;

typedef enum _OD_DATA_TYPE
{
	NORMAL_FIELD = 0,
	HOLEMAP_FIELD,
	FITTER_FIELD,
	DEBUG_FIELD
}OD_DATA_TYPE;

#define OBJECT_DICTIONARY_DATA_SIZE_BYTES 4

void InitialiseCanObjectDictionary(void);

/* Interface functions to be used for CANOpen commands accessing the dictionary -
	these are only capable of a 32 bit read and write based upon a given index,
	they should not be used by the application for tasks other than reading/writing
	to the CAN network or writing from the uplink. */
OBJECT_DICTIONARY_ERROR	SetCANObjectData(uint32_t z_index, uint32_t z_data);
OBJECT_DICTIONARY_ERROR	GetCANObjectData(uint32_t z_index, uint32_t * z_data);
void SettCANObjectData(uint32_t z_index, uint32_t z_data);
OD_DATA_TYPE GetFieldType(uint8_t z_id);

/* Interface functions for housekeeping data access. */
uint8_t GetHousekeepingData(uint8_t ** z_data);

/* Other interface functions. */
void PerformEventActions(uint8_t z_index);

/* These are not to be used for sending data externally - they ignore the changed flag / endianism swap / GET/SET access rights . */
OBJECT_DICTIONARY_ERROR	SetCANObjectDataRaw(uint32_t z_index, uint32_t z_data);
OBJECT_DICTIONARY_ERROR	GetCANObjectDataRaw(uint32_t z_index, uint32_t * z_data);

#endif