#include "Uplink.h"
#include "Demodulator.h"
#include <string.h>
#include "sampling.h"
#include "FreeRTOS.h"
#include "task.h"
#include "CANODInterface.h"
#include "queue.h"
#include <string.h>
#include "timers.h"
#include "Watchdog.h"

#define SAMPLES_PER_EXECUTION_CYCLE 500		/* This is highly tunable depending on processor usage. */
#define PACKET_BUFFER_SIZE 3

int32_t dataInput[SAMPLES_PER_EXECUTION_CYCLE];

uint32_t (*p_UplinkInputBufferGet)(int32_t *) = 0;
uint32_t packetBufferCount = 0;
uint32_t lastCommand = 0;
uint32_t uplinkPacketsDropped = 0;

static QueueHandle_t commandQueueHandle;			/**< The handle of the command queue */

static StaticTimer_t packetCounterUpdateTimer;
static xTimerHandle  packetCounterUpdateTimerHandle;
#define DELAY_1S 1000
#define AUTO_RELOAD 1
static void uplinkUpdateTimerCallback(xTimerHandle xTimer);
static uint8_t uplinkUpdate = 0;

/* Top level function call for initializing the uplink. */
void UplinkInit(void)
{
	memset(dataInput, 0 , sizeof(dataInput));
	packetBufferCount = 0;
	RegisterUplinkInput(&UplinkInputBufferGet);

	/* Get the command queue to post commands to */
	commandQueueHandle =  GetCommandQueueHandle();
	packetCounterUpdateTimerHandle = xTimerCreateStatic("UplinkUpdate", DELAY_1S, AUTO_RELOAD, 0, &uplinkUpdateTimerCallback, &packetCounterUpdateTimer);
	xTimerStart(packetCounterUpdateTimerHandle, 0);
	
	uplinkPacketsDropped = 0;
	packetBufferCount = 0;
	lastCommand = 0;
	InitSampling();
	AFSKInit();	
	RegisterThreadWithManager(UPLINK_THREAD);
}

/* This function registers the function used to get sample data from the ADCs.
   This function registration must be performed in order for the uplink to work.
*/
void RegisterUplinkInput(uint32_t((*z_func)(int32_t *)))
{
	if(z_func != 0)
	{
		p_UplinkInputBufferGet = z_func;
	}
}

void UplinkThread(void)
{
	while(1)
	{
		ProvideThreadHeartbeat(UPLINK_THREAD);
		UplinkExecute();
		vTaskDelay(1);
	}
}
/* This function is the main uplink execution loop */
uint8_t UplinkExecute(void)
{
	uint8_t retVal = 0;
	int32_t bsize, bcount;
	uint32_t i = 0;

	if(p_UplinkInputBufferGet != 0)
	{
		uint32_t i;
		uint32_t bufferCount = 0;

		/* Find out how much we have in the buffer */
		bufferCount = SizeSamplingBuff();

		/* Limit our data fetch to the size of the data we have available. */
		bufferCount = bufferCount > SAMPLES_PER_EXECUTION_CYCLE ? SAMPLES_PER_EXECUTION_CYCLE : bufferCount;

		/* Fetch the data from the receive buffer. */
		for(i = 0; i < bufferCount; i++)
		{
			p_UplinkInputBufferGet(&dataInput[i]);
			dataInput[i] -= 1024;	// Scale to -1024 to 1024
		}

		AFSKDemod(dataInput, bufferCount);

		retVal = 1;
	}
	
	/* Update all counter variables */
	if(uplinkUpdate != 0)
	{
		uplinkUpdate = 0;
		if(commandQueueHandle != 0)	
		{
			uint8_t buffer[CAN_QUEUE_ITEM_SIZE];
			/* Record that this data is from the uplink. */
			buffer[PACKET_SOURCE_INDEX] = ON_TARGET_DATA;
			/* Add the packet length to the queue packet. */
			buffer[PACKET_SIZE_INDEX] = 5;
			buffer[OBJECT_DICTIONARY_ID_INDEX] = AMS_OBC_P_UP;
			memcpy(&buffer[PACKET_DATA_INDEX], &packetBufferCount, sizeof(packetBufferCount));
			xQueueSend(commandQueueHandle, buffer, 0);
			buffer[OBJECT_DICTIONARY_ID_INDEX] = AMS_OBC_LC;
			memcpy(&buffer[PACKET_DATA_INDEX], &lastCommand, sizeof(lastCommand));
			xQueueSend(commandQueueHandle, buffer, 0);
			buffer[OBJECT_DICTIONARY_ID_INDEX] = AMS_OBC_P_UP_DROPPED;
			memcpy(&buffer[PACKET_DATA_INDEX], &uplinkPacketsDropped, sizeof(uplinkPacketsDropped));
			xQueueSend(commandQueueHandle, buffer, 0);
		}	
	}
	return retVal;
}

/* This function will add a decoded uplink packet to the command queue.
   Although this function is in the .h interface file it should not be called
   by anything other than the decoder - lack of time for me to be a purist!
*/
void PutUplinkPacket(UPLINK_PACKET z_packet)
{
	uint8_t buffer[CAN_QUEUE_ITEM_SIZE];
	/* Increment our packet counter. */
	packetBufferCount++;

	/* Don't try to add to the queue if we don't have access to the queue. */
	if(commandQueueHandle != 0)
	{
		/* Clear out output buffer */
		memset(buffer, 0, sizeof(buffer));
		/* Record the last COB ID that we sent. */
		lastCommand = z_packet.data[0];				
		/* Record that this data is from the uplink. */
		buffer[PACKET_SOURCE_INDEX] = OFF_TARGET_DATA;
		/* Add the packet length to the queue packet. */
		buffer[PACKET_SIZE_INDEX] = z_packet.packetLength;
		/* Copy across the data we have received. */
		memcpy(&buffer[OBJECT_DICTIONARY_ID_INDEX], &z_packet.data[0], sizeof(z_packet.data));

		xQueueSend(commandQueueHandle, buffer, 0);
	}
	
	/* We have received a packet so kick the watchdog */
	KickUplinkCommandWatchdog();
}

void PacketRejected(void)
{
	uplinkPacketsDropped++;
}
/* Get the number of packets we have received since start up */
uint8_t GetUplinkPacketCount(void)
{
	return packetBufferCount;
}

static void uplinkUpdateTimerCallback(xTimerHandle xTimer)
{
	uplinkUpdate = 1;
}
