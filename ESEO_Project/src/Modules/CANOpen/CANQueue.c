#include "CANODInterface.h"
#include "CANObjectDictionary.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "CANQueue.h"
#include <string.h>

static QueueHandle_t queueHandle = 0;
static StaticQueue_t queueControl;
#define QUEUE_LENGTH 50
#define QUEUE_ITEM_SIZE 25
uint8_t queueStorage[QUEUE_LENGTH * QUEUE_ITEM_SIZE];

#define MAXIMUM_REGULAR_PACKET_SIZE 5		// The maximum allowed size for a regular (not fitter/holemap/debug) packet.
#define MAXIMUM_FITTER_PACKET_SIZE	21		// Maximum allowed size for a fitter message packet.
#define MAXIMUM_HOLEMAP_PACKET_SIZE	21		// Maximum allowed size for a holemap packet.
#define MAXIMUM_DEBUG_PACKET_SIZE	21		// Maximum allowed size for a debug packet.
#define NUMBER_OF_COMMANDS_TO_SERVICE 30	// The maximum number of commands to service per iteration.

static void RouteReceivedPacket(uint8_t * z_packet);
static void UpdateHolemapAndFitterFields(uint8_t * z_packet);

/* Initialise the CAN OD message queue */
void InitialiseCANQueue(void)
{
	queueHandle = xQueueCreateStatic(QUEUE_LENGTH, QUEUE_ITEM_SIZE, queueStorage, &queueControl);	
}

/* This function will look at the CAN OD queue and service any packets present. */
void ServiceCommandQueue(void)
{
	uint8_t buffer[QUEUE_ITEM_SIZE];
	uint32_t i = 0;
	
	/* Ensure we have a queue to fetch data from */
	if(queueHandle != 0)
	{
		for(i = 0; i < NUMBER_OF_COMMANDS_TO_SERVICE; i++)
		{
			/* Have we got any requests waiting for us? */
			if(uxQueueMessagesWaiting(queueHandle))
			{
				/* Extract the packet and place it into buffer. */
				if(xQueueReceive(queueHandle, buffer, 0))
				{
					/* Different packets need to be handled differently */
					RouteReceivedPacket(buffer);
				}
			}
			else
			{
				break;
			}			
		}
	}	
	else
	{
		/* Error handler needs to do something here. */
	}
}

/* This function looks at the field that the command is trying to write to
   and then dispatches to the correct function to handle it. */
static void RouteReceivedPacket(uint8_t * z_packet)
{
	if(z_packet != 0)
	{
		/* We impose a different maximum packet length for different data types. */
		switch(GetFieldType(z_packet[OBJECT_DICTIONARY_ID_INDEX]))
		{
			case NORMAL_FIELD:
			{
				/* Is the packet the expected size? */
				if(z_packet[PACKET_SIZE_INDEX] <= MAXIMUM_REGULAR_PACKET_SIZE)
				{
					if(z_packet[PACKET_SOURCE_INDEX] == OFF_TARGET_DATA)
					{
						uint32_t store = 0;
						memcpy(&store, &z_packet[PACKET_DATA_INDEX], sizeof(uint32_t));						
						/* Update the OD. */
						SetCANObjectData(z_packet[OBJECT_DICTIONARY_ID_INDEX], store);
					}
					else if(z_packet[PACKET_SOURCE_INDEX] == ON_TARGET_DATA)
					{
						uint32_t store = 0;
						memcpy(&store, &z_packet[PACKET_DATA_INDEX], sizeof(uint32_t));
						/* Update the OD. */
						SetCANObjectDataRaw(z_packet[OBJECT_DICTIONARY_ID_INDEX], store);
					}
				}
			}
			break;
			case HOLEMAP_FIELD:
			{
				/* Is the packet the expected size? */
				if(z_packet[PACKET_SIZE_INDEX] <= MAXIMUM_HOLEMAP_PACKET_SIZE)
				{
					UpdateHolemapAndFitterFields(z_packet);
				}
				break;
			}
			case FITTER_FIELD:
			{
				/* Is the packet the expected size? */
				if(z_packet[PACKET_SIZE_INDEX] <= MAXIMUM_FITTER_PACKET_SIZE)
				{
					UpdateHolemapAndFitterFields(z_packet);
				}
				break;
			}
			case DEBUG_FIELD:
			{
				/* Is the packet the expected size? */
				if(z_packet[PACKET_SIZE_INDEX] <= MAXIMUM_DEBUG_PACKET_SIZE)
				{
					uint32_t store = 0;
					memcpy(&store, &z_packet[PACKET_DATA_INDEX], sizeof(uint32_t));					
					/* Update the OD - this is wrong for now as the command list is undefined. */
					SetCANObjectData(z_packet[OBJECT_DICTIONARY_ID_INDEX], store);
				}
				break;
			}
		}
	}
}

/* Add the stream of data to the fitter message / holemap -
   data is assumed to have been verified by this point! */
static void UpdateHolemapAndFitterFields(uint8_t * z_packet)
{
	uint32_t i = 0;

	uint32_t packetDataFieldsInBytes = (z_packet[PACKET_SIZE_INDEX] - 1);	// Minus one because packet size includes the CAN OD ID
	uint32_t bytesRemaining = packetDataFieldsInBytes % sizeof(uint32_t);	// How far off a 32 bit boundary we are.
	uint32_t packetDataToTransferInWords = (packetDataFieldsInBytes + bytesRemaining) / sizeof(uint32_t);	// Convert from bytes into words to add to CAN OD.

	for(i = 0; i < packetDataToTransferInWords; i++)
	{
		uint32_t store = 0;
		/* Take 32 bit chunks at a time. */
		memcpy(&store, &z_packet[PACKET_DATA_INDEX + (i*sizeof(uint32_t))], sizeof(uint32_t));		
		if(z_packet[PACKET_SOURCE_INDEX] == OFF_TARGET_DATA)
		{
			SetCANObjectData(z_packet[OBJECT_DICTIONARY_ID_INDEX], store);
		}
		else if(z_packet[PACKET_SOURCE_INDEX] == ON_TARGET_DATA)
		{
			SetCANObjectDataRaw(z_packet[OBJECT_DICTIONARY_ID_INDEX], store);
		}
	}
}

/* 	This is an interface function to allow other threads 
	to get ahold of the queue for writing to the OD. */
QueueHandle_t GetCommandQueueHandle(void)
{
	return queueHandle;
}
