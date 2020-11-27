/* There was a requirement to add some virtual memory addressing to the CAN OD,
to save space this has been done in a non generic way as it would have been
much too large otherwise. This is handled in this file - holemaps and fitter
messages live in memory in this file. */
#include "ExtendedObjectDictionaryFields.h"
#include "CANODInterface.h"
#include "CANObjectDictionary.h"
#include "semphr.h"
#include <string.h>

#define HOLEMAP_SIZE 64
#define FITTER_MESSAGE_SIZE 50
#define NUMBER_OF_FITTER_MESSAGES 4

#define HOLEMAP_SEMAPHORE_WAIT_TICKS	2

uint32_t holemapStorge[HOLEMAP_SIZE] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
										0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

uint32_t fitterMessageStorage[NUMBER_OF_FITTER_MESSAGES][FITTER_MESSAGE_SIZE] = 
{
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

//{
//	{0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA},
//	{0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA},
//	{0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA}
//};

static SemaphoreHandle_t canObjectDictionarySemaphore;

void InitialiseExtendedObjectFields(SemaphoreHandle_t z_canObjectDictionarySemaphore)
{
	/* Save our semaphore handle for later. */
	canObjectDictionarySemaphore = z_canObjectDictionarySemaphore;
}

/* Clear all of the data in the holemap */
void ClearHoleMap(void)
{
	if(canObjectDictionarySemaphore != 0)
	{
		if(xSemaphoreTakeRecursive(canObjectDictionarySemaphore, HOLEMAP_SEMAPHORE_WAIT_TICKS))
		{
			memset(holemapStorge, 0 , sizeof(holemapStorge));

		}
		xSemaphoreGiveRecursive(canObjectDictionarySemaphore);
	}
}

/* Gets the current holemap in its entirety from the buffer */
void GetHolemap(uint8_t * z_buffer, uint16_t z_size)
{
	if(canObjectDictionarySemaphore != 0)
	{
		if(xSemaphoreTakeRecursive(canObjectDictionarySemaphore, HOLEMAP_SEMAPHORE_WAIT_TICKS))
		{
			if(z_size >= sizeof(holemapStorge) && z_buffer != 0)
			{
				memcpy(z_buffer, holemapStorge, z_size);
			}
			xSemaphoreGiveRecursive(canObjectDictionarySemaphore);
		}
	}
}

/* Gets a current fitter message in its entirety from the buffer */
void GetFitterMessage(uint8_t z_slot, uint8_t * z_buffer, uint16_t z_size)
{
	if(z_size >= (sizeof(fitterMessageStorage) / NUMBER_OF_FITTER_MESSAGES) && z_buffer != 0)
	{
		if(canObjectDictionarySemaphore != 0)
		{
			if(xSemaphoreTakeRecursive(canObjectDictionarySemaphore, HOLEMAP_SEMAPHORE_WAIT_TICKS))
			{
				if(z_slot < NUMBER_OF_FITTER_MESSAGES)
				{
					memcpy(z_buffer, &fitterMessageStorage[z_slot][0], z_size);
				}

			}
			xSemaphoreGiveRecursive(canObjectDictionarySemaphore);
		}
	}
}

/* Function that when called sets a 32 bit field in the holemap */
CAN_OPEN_ERROR SetHolemapField(uint8_t z_id, uint32_t z_data, uint8_t z_changed)
{
	uint32_t index;

	if(canObjectDictionarySemaphore != 0)
	{
		if(xSemaphoreTakeRecursive(canObjectDictionarySemaphore, HOLEMAP_SEMAPHORE_WAIT_TICKS))
		{
			/* Go fetch the current holemap index. */
			GetCANObjectDataRaw(AMS_OBC_HM_INDEX, &index);

			/* Reached the end of the map so we loop */
			if(index >= HOLEMAP_SIZE)
			{
				index = 0;
			}

			/* Get the requested holemap field */
			holemapStorge[index] = z_data;

			/* Update the index for next time. */
			index++;
			SetCANObjectDataRaw(AMS_OBC_HM_INDEX, index);

			xSemaphoreGiveRecursive(canObjectDictionarySemaphore);
		}
	}
	return NO_ERROR_CAN_OPEN;
}

/* Gets a 32 bit field from the holemap */
CAN_OPEN_ERROR GetHolemapField(uint8_t z_id, uint32_t * z_data)
{
	uint32_t index;

	if(canObjectDictionarySemaphore != 0)
	{
		if(xSemaphoreTakeRecursive(canObjectDictionarySemaphore, HOLEMAP_SEMAPHORE_WAIT_TICKS))
		{
			/* Go fetch the current holemap index. */
			GetCANObjectDataRaw(AMS_OBC_HM_INDEX, &index);

			/* Reached the end of the map so we loop */
			if(index >= HOLEMAP_SIZE)
			{
				index = 0;
			}

			/* Get the requested holemap field */
			*z_data = holemapStorge[index];

			/* Update the index for next time. */
			index++;
			SetCANObjectDataRaw(AMS_OBC_HM_INDEX, index);

			xSemaphoreGiveRecursive(canObjectDictionarySemaphore);
		}
	}

	return NO_ERROR_CAN_OPEN;
}

/* Changes between the three available fitter message slots - limited to the number of slots, 
   if a value greater than the number of slots is provided then we change to the maximum 
   available slot. */
CAN_OPEN_ERROR ChangeFitterMessageSlot(uint8_t z_id, uint32_t z_data, uint8_t z_changed)
{
	if(z_changed != 0)
	{
		/* Limit this field to the number of slots available */
		if(z_data < NUMBER_OF_FITTER_MESSAGES)
		{
			SetCANObjectDataRaw(AMS_OBC_FM_SLOT, z_data);
		}
		else
		{
			SetCANObjectDataRaw(AMS_OBC_FM_SLOT, (NUMBER_OF_FITTER_MESSAGES-1));
		}
		/* We have changed the fitter slot number so reset the index. */
		SetCANObjectDataRaw(AMS_OBC_FM_INDEX, 0);
	}

	return NO_ERROR_CAN_OPEN;
}

/* Sets a 32 bit value in the fitter message field - which slot is used is already
   configured in the CAN OD.*/
CAN_OPEN_ERROR SetFitterMessageField(uint8_t z_id, uint32_t z_data, uint8_t z_changed)
{
	uint32_t fitterMessageNumber;
	uint32_t index;
	CAN_OPEN_ERROR retVal = COULD_NOT_EXECUTE_ACTION;

	/* Go fetch the current fitter index. */
	GetCANObjectDataRaw(AMS_OBC_FM_INDEX, &index);

	/* Go fetch the current fitter index. */
	GetCANObjectDataRaw(AMS_OBC_FM_SLOT, &fitterMessageNumber);

	if(canObjectDictionarySemaphore != 0)
	{
		if(xSemaphoreTakeRecursive(canObjectDictionarySemaphore, HOLEMAP_SEMAPHORE_WAIT_TICKS))
		{
			if(fitterMessageNumber < NUMBER_OF_FITTER_MESSAGES)
			{
				/* Reached the end of the map so we loop */
				if(index >= FITTER_MESSAGE_SIZE)
				{
					index = 0;
				}

				/* Get the requested holemap field */
				fitterMessageStorage[fitterMessageNumber][index] = z_data;

				/* Update the index for next time. */
				index++;
				SetCANObjectDataRaw(AMS_OBC_FM_INDEX, index);

				retVal = NO_ERROR_CAN_OPEN;
			}
			xSemaphoreGiveRecursive(canObjectDictionarySemaphore);
		}
	}
	return retVal;
}

/* Gets a 32 bit value in the fitter message field - which slot is used is already
   configured in the CAN OD.*/
CAN_OPEN_ERROR GetFitterMessageField(uint8_t z_id, uint32_t * z_data)
{
	uint32_t fitterMessageNumber;
	uint32_t index;
	CAN_OPEN_ERROR retVal = COULD_NOT_EXECUTE_ACTION;

	/* Go fetch the current fitter index. */
	GetCANObjectDataRaw(AMS_OBC_FM_INDEX, &index);

	/* Go fetch the current fitter index. */
	GetCANObjectDataRaw(AMS_OBC_FM_SLOT, &fitterMessageNumber);

	if(canObjectDictionarySemaphore != 0)
	{
		if(xSemaphoreTakeRecursive(canObjectDictionarySemaphore, HOLEMAP_SEMAPHORE_WAIT_TICKS))
		{
			if(fitterMessageNumber < NUMBER_OF_FITTER_MESSAGES)
			{

				/* Reached the end of the map so we loop */
				if(index >= FITTER_MESSAGE_SIZE)
				{
					index = 0;
				}

				/* Get the requested fitter field */
				*z_data = fitterMessageStorage[fitterMessageNumber][index];

				/* Update the fitter for next time. */
				index++;
				SetCANObjectDataRaw(AMS_OBC_FM_INDEX, index);
			}
			xSemaphoreGiveRecursive(canObjectDictionarySemaphore);
		}
	}
	return retVal;
}