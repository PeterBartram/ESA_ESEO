#ifndef PAYLOADDATA_H_
#define PAYLOADDATA_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

/* Queue defines. */
#define PAYLOAD_DATA_QUEUE_LENGTH		100		// The number of items we can have on the queue at a given time.
#define PAYLOAD_DATA_QUEUE_ITEM_SIZE	5		// The size of a single item on the payload queue
#define PAYLOAD_QUEUE_TICKS_TO_WAIT		2		// The number of ticks to wait on a message from the queue.
#define PAYLOAD_PACKET_CONTROL_INDEX	0		// Index into the packet for the control/data switch field.
#define PAYLOAD_PACKET_DATA_INDEX		1		// Index into the packet for the data portion
#define PAYLOAD_PACKET_COMMAND_INDEX	1		// Index into the packet for the command field.
#define PAYLOAD_COMMAND_PACKET			0		// Zero in PAYLOAD_PACKET_CONTROL_INDEX means this is a command packet.
#define PAYLOAD_DATA_PACKET				1		// One in PAYLOAD_PACKET_CONTROL_INDEX means this is a data packet.

/* Command defines - used in the message queue field */
#define PAYLOAD_DATA_TRANSFER_STOP		0		// Stop a payload data transfer
#define PAYLOAD_DATA_TRANSFER_START		1		// Start a payload data transfer
#define PAYLOAD_DATA_TRANSFER_ABORT		2		// Abort a payload data transfer

typedef enum _PAYLOAD_DATA_ERROR
{
	NO_ERROR_PAYLOAD_DATA = 0,			
	NO_DATA_READY,						// No data ready to be downlinked.
	NOT_PROCESSING_PAYLOAD_DATA,		// Not currently dealing with payload data.
	INVALID_INPUT,						// We have been given an invalid input.
	SEMAPHORE_TIMED_OUT					// Could not take the semaphore
}PAYLOAD_DATA_ERROR;

void InitialiseFlash(void);
void ClearPayloadFlash(void);
SemaphoreHandle_t InitialiseFlashAccessProtection(void);
void InitialisePayloadData(void);
void PayloadDataThread(void);
uint8_t PayloadDataTask(void);
QueueHandle_t GetPayloadDataQueueHandle(void);
PAYLOAD_DATA_ERROR GetPayloadData(uint8_t * z_data, uint16_t z_size, uint16_t * z_packetNumber);
uint8_t PayloadDataPresent(void);

#endif /* PAYLOADDATA_H_ */
