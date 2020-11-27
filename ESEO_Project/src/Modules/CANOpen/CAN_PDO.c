#include "CAN_PDO.h"
#include "string.h"
#include "timers.h"
#include "PayloadData.h"
#include "CANODInterface.h"

/* Permanent defines - don't change these! */
#define PDO_PACKET_DATA_SIZE 6
#define PDO_SET_ACK_PACKET_SIZE 6
#define CAN_DATA_FIELD
#define ACTIVE_PAYLOAD_INPUT_MASK	0x003F0000		/**< Used for converting from the 32 bit value provided into an 8 bit value for use. */
#define ACTIVE_PAYLOAD_SHIFT 16						/**< Used for converting from the 32 bit value provided into an 8 bit value for use. */
#define	DATA_TRANSFER_CONFIG_ADDRESS 0				/**< The object dictionary address containing the configuration registers on a payload. */
#define MAX_TRANSFERS_PER_CYCLE 512					/**< The maximum number of transfers permitted in any one OBDH cycle. */
#define TICKS_TO_WAIT_ON_DATA_QUEUE	2				/**< The maximum number of ticks to wait on sending data to the queue */

/* Indexes into CAN data field. */
#define ADDRESS_INDEX 0
#define SEQUENCE_NUMBER_INDEX 1
#define PACKET_DATA_INDEX 2

/* Indexes into payload data transfer protocol field. */
#define NODE_ID 0x2
#define DATA_COUNTER 0x4

/* Defines for payload register access. */
#define CONFIGURATION_REGISTER 0x00
#define DATA_REGISTER_ONE 0x01
#define DATA_REGISTER_TWO 0X02

/* Bit identifier masks for payloads */
#define	ACTIVE_PAYLOAD_PCAM		0x1
#define	ACTIVE_PAYLOAD_LMD		0x2
#define	ACTIVE_PAYLOAD_TRI		0x4
#define	ACTIVE_PAYLOAD_GPS		0x8
#define	ACTIVE_PAYLOAD_OBDH		0x10
#define	ACTIVE_PAYLOAD_SCAM		0x20

/* Delay defines. */
#define DELAY_10_MS  (10/portTICK_RATE_MS)

/* These are the possible states of a payload transfer operation. */
typedef enum
{
	TRANSFER_IDLE = 0,			/**< We are idle in terms of payload data transfers. */
	TRANSFER_INITIALISE,		/**< We have been told to initialise a payload transfer. */
	TRANSFER_INITIALISING,		/**< We have kicked off a transfer and are awaiting a response from a slave. */
	TRANSFER_SEND,				/**< We are required to send a PDOGet to collect data. */
	TRANSFER_RECEIVE,			/**< We are awaiting a response to a PDOGet. */
	TRANSFER_PAUSED,			/**< The transfer is paused but still active. */
	TRANSFER_COMPLETE,			/**< Transfer complete - execute any required actions. */
	TRANSFER_ERROR				/**< An error has occurred, reset the transfer system. */
}PAYLOAD_TRANSFER_STATE;

/* Object containing a single transfer operations configuration data. */
typedef struct 
{
	uint8_t payload;
	PAYLOAD_TRANSFER_STATE	state;
	uint16_t transmitChannel;
	uint16_t receiveChannel;
	uint16_t packetsRemaining;
	uint8_t sequenceNumber;
	uint8_t nodeID;
	uint8_t readRegister;
	uint8_t renegotiate;
	uint16_t cycleTransfers;
	uint16_t timeOut;
	xTimerHandle timer;
}PAYLOAD_DATA_TRANSFER_CONTROL;

PAYLOAD_DATA_TRANSFER_CONTROL transferControl	=
{
	0,
	TRANSFER_IDLE,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	(void*)0
};

/* Private functions. */
static uint8_t SetActivePayload(uint8_t z_payload);
static void ResetPayloadDataTransferProtocol(void);
static void TimerExpiredCallback(xTimerHandle xTimer);
static CAN_OPEN_ERROR PayloadTransferProtocolInitialise(void);
static void ChangeToErrorIfTimeoutHasOccurred(void);
static CAN_OPEN_ERROR PayloadTransferProtocolSend(void);
static uint8_t ReinitialiseIfWeHaveBeenToldToRenegotiate(void);
static void PayloadDataTransferProtocolAcknowledgeInitialise(const CAN_PACKET * z_incomingPacket);
static void PayloadDataTransferProtocolAcknowledgeReceive(const CAN_PACKET * z_incomingPacket);

static StaticTimer_t canOpenTimeOutTimer;
static QueueHandle_t payloadStorageQueue = 0;
static uint32_t packetsTransferred = 0;

CAN_OPEN_ERROR InitialiseProcessDataObjects(QueueHandle_t z_payloadStorageQueue)
{
	/* Initialise our transfer process. */
	ResetPayloadDataTransferProtocol();

	/* We want to create a timer to handle initialisation timeout here. */
	transferControl.timer = xTimerCreateStatic("CANOpenTimers", DELAY_10_MS, 0, 0, &TimerExpiredCallback, &canOpenTimeOutTimer);

	payloadStorageQueue = z_payloadStorageQueue;

	return transferControl.timer ? NO_ERROR_CAN_OPEN : INITIALISATION_FAILED_CAN_OPEN;
}

CAN_OPEN_ERROR	ProcessDataObjectGET(const CAN_PACKET * z_incomingPacket, CAN_PACKET * z_outgoingPacket)
{
	CAN_OPEN_ERROR retVal = NO_ERROR_CAN_OPEN;
	uint32_t readBuffer = 0;

	if(z_incomingPacket && z_outgoingPacket)
	{
		if(z_incomingPacket->dataSize == PDO_PACKET_DATA_SIZE)
		{
			/* Fetch the data from the object dictionary. */
			if(GetCANObjectData(z_incomingPacket->data[ADDRESS_INDEX], &readBuffer) == NO_ERROR_CAN_OPEN)
			{
				/* Copy the packet data into the output packet (compiler is funny about a direct write in function call above. */
				memcpy(&z_outgoingPacket->data[PACKET_DATA_INDEX], &readBuffer, OBJECT_DICTIONARY_DATA_SIZE_BYTES);
				
				/* The ACK sequence number should match the recevied sequence number. */
				z_outgoingPacket->data[SEQUENCE_NUMBER_INDEX] = z_incomingPacket->data[SEQUENCE_NUMBER_INDEX];

				/* Return address same as incoming address. */
				z_outgoingPacket->data[ADDRESS_INDEX] = z_incomingPacket->data[ADDRESS_INDEX];

				/* Configure the packet size */
				z_outgoingPacket->dataSize = PDO_PACKET_DATA_SIZE;

				/* Configure the reply to use the correct connection object ID. */
				z_outgoingPacket->id = COB_ID_PDO_ACK;

				/* Send the ACK as a data frame. */
				z_outgoingPacket->requestType = DATA_FRAME;
			}
			else
			{
				/* Awaiting instuction from ESA as to what behaviour to implement here. */
				retVal = INVALID_PDO_PACKET;
			}
		}
		else
		{
			retVal = INVALID_PDO_PACKET;
		}
	}
	else
	{
		retVal = NULL_POINTER_CAN_OPEN;
	}
	return retVal;
}

CAN_OPEN_ERROR	ProcessDataObjectSET(const CAN_PACKET * z_incomingPacket, CAN_PACKET * z_outgoingPacket)
{
	CAN_OPEN_ERROR retVal = NO_ERROR_CAN_OPEN;

	if(z_incomingPacket && z_outgoingPacket)
	{
		if(z_incomingPacket->dataSize == PDO_PACKET_DATA_SIZE)
		{
			uint32_t dataBuffer = 0;
			
			/* Copy our data into a buffer to later use. */
			memcpy(&dataBuffer, &z_incomingPacket->data[PACKET_DATA_INDEX], sizeof(uint32_t));
			
			/* Set the requested object in our dictionary */
			if(SetCANObjectData(z_incomingPacket->data[ADDRESS_INDEX], dataBuffer)  == NO_ERROR_CAN_OPEN)
			{
				/* The ACK sequence number should match the recevied sequence number. */
				z_outgoingPacket->data[SEQUENCE_NUMBER_INDEX] = z_incomingPacket->data[SEQUENCE_NUMBER_INDEX];

				/* Return address same as incoming address. */
				z_outgoingPacket->data[ADDRESS_INDEX] = z_incomingPacket->data[ADDRESS_INDEX];

				/* Configure the packet size */
				z_outgoingPacket->dataSize = PDO_SET_ACK_PACKET_SIZE;

				/* Configure the reply to use the correct connection object ID. */
				z_outgoingPacket->id = COB_ID_PDO_ACK;

				/* Send the ACK as a data frame. */
				z_outgoingPacket->requestType = DATA_FRAME;

				/* A SET ACK should mirror the data that was sent to it. */
				memcpy(&z_outgoingPacket->data[PACKET_DATA_INDEX], &dataBuffer, sizeof(uint32_t));
			}
			else
			{
				/* Awaiting instruction from ESA as to what behavior to implement here. */
			}
		}
		else
		{
			retVal = INVALID_PDO_PACKET;
		}
	}
	else
	{
		retVal = NULL_POINTER_CAN_OPEN;
	}
	return retVal;
}

void ProcessDataObjectSETT(const CAN_PACKET * z_incomingPacket)
{
	if(z_incomingPacket)
	{
		if(z_incomingPacket->dataSize == PDO_PACKET_DATA_SIZE)
		{
			uint32_t dataBuffer = 0;
			
			/* Copy our data into a buffer to later use. */
			memcpy(&dataBuffer, &z_incomingPacket->data[PACKET_DATA_INDEX], sizeof(uint32_t));

			/* Update the time. */
			SettCANObjectData(z_incomingPacket->data[ADDRESS_INDEX], dataBuffer);
		}
	}
}

CAN_OPEN_ERROR ReceivedPayloadDataTransferProtocolRequest(uint8_t z_id, uint32_t z_data, uint8_t z_changed)
{
	/* Extract the active payload field from our incoming data. */
	uint8_t activePayload = (uint8_t)((z_data  & ACTIVE_PAYLOAD_INPUT_MASK) >> ACTIVE_PAYLOAD_SHIFT);
	uint8_t buffer[PAYLOAD_DATA_QUEUE_ITEM_SIZE];

	if(activePayload)
	{
		/* If we have changed to a new payload to download from. */
		if(SetActivePayload(activePayload))
		{

			/* We need to clear all holemap data at this point. */

			/* Check if we have a queue to post to. */
			if(payloadStorageQueue != 0)
			{
				/* Create a transfer start packet. */
				buffer[PAYLOAD_PACKET_CONTROL_INDEX] = PAYLOAD_COMMAND_PACKET;
				buffer[PAYLOAD_PACKET_DATA_INDEX] = PAYLOAD_DATA_TRANSFER_START;

				/* We have received a valid start request tell the flash manager to prepare to receive data. */
				xQueueSend(payloadStorageQueue, &buffer[0], TICKS_TO_WAIT_ON_DATA_QUEUE);

				/* Set our packets transferred counter to zero. */
				packetsTransferred = 0;
				/* Record the payload that we are transferring from */
				SetCANObjectDataRaw(AMS_OBC_CAN_PL, activePayload);
				
				/* Record that we are now transferring from a payload. */
				SetCANObjectDataRaw(AMS_OBC_PL_TRANS_STAT, 0);
				ClearHoleMap();
			}

			/* We now need to initialise the transfer process. */
			transferControl.state = TRANSFER_INITIALISE;
		}
		else
		{
			/* We need to set a flag to reinitialise the process */
			transferControl.renegotiate = 1;
		}
	}
	else
	{
		transferControl.state = TRANSFER_IDLE;
	}

	return NO_ERROR_CAN_OPEN;
}


CAN_OPEN_ERROR CANPDOPayloadTransferProtocolUpdate(void)
{
	/* This is the main payload data transfer protocol state machine. */
	switch(transferControl.state)
	{
		case TRANSFER_INITIALISE:
			PayloadTransferProtocolInitialise();
			break;
		case TRANSFER_INITIALISING:
			ChangeToErrorIfTimeoutHasOccurred();
			break;
		case TRANSFER_SEND:
			PayloadTransferProtocolSend();
			break;
		case TRANSFER_RECEIVE:
			ChangeToErrorIfTimeoutHasOccurred();
			break;
		case TRANSFER_COMPLETE:
			ResetPayloadDataTransferProtocol();
			break;
		case TRANSFER_ERROR:
			ResetPayloadDataTransferProtocol();
			break;
		case TRANSFER_PAUSED:
			ReinitialiseIfWeHaveBeenToldToRenegotiate();
			break;
		default:
			break;
	}

	return NO_ERROR_CAN_OPEN;
}

CAN_OPEN_ERROR ReceivedPayloadDataTransferProtocolAcknowledge(const CAN_PACKET * z_incomingPacket)
{
	/* If this acknowledge is from an enexpected COB-ID then we ignore. */
	if(z_incomingPacket->id == transferControl.receiveChannel)
	{
		/* Ensure the packet we have received is as expected. */
		if(z_incomingPacket->dataSize == PDO_PACKET_DATA_SIZE)
		{
			/* A correctly formatted acknowledgement has been received */
			switch(transferControl.state)
			{
				case TRANSFER_INITIALISING:
					PayloadDataTransferProtocolAcknowledgeInitialise(z_incomingPacket);
					break;
				case TRANSFER_RECEIVE:
 					PayloadDataTransferProtocolAcknowledgeReceive(z_incomingPacket);
					break;
				/* Added these statements for testing. @todo should probably change this!  */
				case TRANSFER_INITIALISE:	break;
				case TRANSFER_PAUSED:		break;	
				default:
					/* Should never happen but bomb out for this error. */
					transferControl.state = TRANSFER_ERROR;
					break;
			}
		}
		else
		{
			/* Go straight to error for invalid data. */
			transferControl.state = TRANSFER_ERROR;
		}		
	}
	return NO_ERROR_CAN_OPEN;
}

static CAN_OPEN_ERROR PayloadTransferProtocolInitialise(void)
{
	CAN_PACKET outgoingPacket = {0};
	CAN_OPEN_ERROR retVal = INITIALISATION_FAILED_CAN_OPEN;

	/* Configure and send the initialisation kick off process. */
	outgoingPacket.id = transferControl.transmitChannel;
	outgoingPacket.data[ADDRESS_INDEX] = CONFIGURATION_REGISTER;
	outgoingPacket.data[SEQUENCE_NUMBER_INDEX] = transferControl.sequenceNumber;
	outgoingPacket.dataSize = PDO_PACKET_DATA_SIZE;
	TransmitCAN(outgoingPacket);
	transferControl.state = TRANSFER_INITIALISING;

	/* Start our timeout period. */
	if(transferControl.timer != 0)
	{
		/* We need to stop our timer here. */
		if(xTimerReset(transferControl.timer, 20) == pdPASS)
		{
			retVal = NO_ERROR_CAN_OPEN;
		}
		else
		{
			/* Something has gone very wrong with freertos - deal with it. */
		}
	}
	return retVal;
}

static CAN_OPEN_ERROR PayloadTransferProtocolSend(void)
{
	CAN_OPEN_ERROR retVal = COULD_NOT_EXECUTE_ACTION;
	CAN_PACKET outgoingPacket = {0};

	if(ReinitialiseIfWeHaveBeenToldToRenegotiate())
	{
	}
	else if(transferControl.cycleTransfers >= MAX_TRANSFERS_PER_CYCLE)
	{
		/* We have reached our maximum transfers for this cycle - pause transfer */
		transferControl.state = TRANSFER_PAUSED;
	}
	else if(transferControl.packetsRemaining)
	{
		/* If we have data to transfer. */
		outgoingPacket.id = transferControl.transmitChannel;
		outgoingPacket.dataSize = PDO_PACKET_DATA_SIZE;
		outgoingPacket.data[ADDRESS_INDEX] = transferControl.readRegister;
		outgoingPacket.data[SEQUENCE_NUMBER_INDEX] = transferControl.sequenceNumber;
		TransmitCAN(outgoingPacket);

		/* Start our timeout period. */
		if(transferControl.timer != 0)
		{
			/* We need to stop our timer here. */
			if(xTimerReset(transferControl.timer, 20) == pdPASS)
			{
				retVal = NO_ERROR_CAN_OPEN;
			}
			else
			{
				/* Something has gone very wrong with freertos - deal with it. */
			}
		}
		transferControl.state = TRANSFER_RECEIVE;
	}
	return retVal;
}

static void PayloadDataTransferProtocolAcknowledgeInitialise(const CAN_PACKET * z_incomingPacket)
{
	if(z_incomingPacket->data[NODE_ID] == transferControl.nodeID && z_incomingPacket->data[ADDRESS_INDEX] == DATA_TRANSFER_CONFIG_ADDRESS)
	{
		memcpy(&transferControl.packetsRemaining, &z_incomingPacket->data[DATA_COUNTER], sizeof(transferControl.packetsRemaining));
		if(transferControl.packetsRemaining > 0)
		{
			/* Increment the sequence number as per requirements */
			transferControl.sequenceNumber++;

			/* New cycle, new cycle transfer count. */
			transferControl.cycleTransfers = 0;

			transferControl.state = TRANSFER_SEND;
		}
		else
		{
			transferControl.state = TRANSFER_IDLE;
		}
	}
	else
	{
		/* Go straight to error for invalid data. */
		transferControl.state = TRANSFER_ERROR;
	}
}

static void PayloadDataTransferProtocolAcknowledgeReceive(const CAN_PACKET * z_incomingPacket)
{
	uint8_t buffer[PAYLOAD_DATA_QUEUE_ITEM_SIZE];

	if(z_incomingPacket->data[ADDRESS_INDEX] == transferControl.readRegister && z_incomingPacket->data[SEQUENCE_NUMBER_INDEX] == transferControl.sequenceNumber)
	{
		/* Decrement the number of packets we have left to acquire. */
		transferControl.packetsRemaining --;

		/* Increment our counter for this cycle. */
		transferControl.cycleTransfers++;

		if(payloadStorageQueue != 0)
		{
			/* Mark this packet as a data packet. */
			buffer[PAYLOAD_PACKET_CONTROL_INDEX] = PAYLOAD_DATA_PACKET;
			
			/* Copy the data across from the payload. */
			memcpy(&buffer[PAYLOAD_PACKET_DATA_INDEX], &z_incomingPacket->data[PACKET_DATA_INDEX], sizeof(uint32_t));

			/* We have received a data packet, so add this to the flash manager queue for storage */
			xQueueSend(payloadStorageQueue, &buffer[0], TICKS_TO_WAIT_ON_DATA_QUEUE);
			
			/* Record that we have added another packet to the queue. */
			packetsTransferred++;
			
			/* Record that we are now transmitting to ground. */
			SetCANObjectDataRaw(AMS_OBC_PL_TRANS_STAT, 1);
			
			/* Update the CAN OD so that it knows how many packets are left to transfer. */
			SetCANObjectDataRaw(AMS_OBC_CAN_PL_PCKTS_REM, transferControl.packetsRemaining);
		}
		/* Are we finished getting data from this payload? */
		if(transferControl.packetsRemaining == 0)
		{
			transferControl.state = TRANSFER_COMPLETE;

			if(payloadStorageQueue != 0)
			{
				/* We have finished transferring from the payload so tell this to the flash manager. */
				/* Create a transfer start packet. */
				buffer[PAYLOAD_PACKET_CONTROL_INDEX] = PAYLOAD_COMMAND_PACKET;
				buffer[PAYLOAD_PACKET_DATA_INDEX] = PAYLOAD_DATA_TRANSFER_STOP;

				/* We have received a valid start request tell the flash manager to prepare to receive data. */
				xQueueSend(payloadStorageQueue, &buffer[0], TICKS_TO_WAIT_ON_DATA_QUEUE);
			}
		}
		else
		{
			/* Increment the sequence number as per requirements */
			transferControl.sequenceNumber++;

			/* We need to read from the other data register next. */
			transferControl.readRegister = (transferControl.readRegister == DATA_REGISTER_TWO) ? DATA_REGISTER_ONE : DATA_REGISTER_TWO;

			/* Not finished yet - send another GETP to the payload. */
			transferControl.state = TRANSFER_SEND;
		}
	}
	else
	{
		transferControl.state = TRANSFER_ERROR;
	}			
}

static uint8_t ReinitialiseIfWeHaveBeenToldToRenegotiate(void)
{
	uint8_t retVal = 0;

	if(transferControl.renegotiate)
	{
		/* We have been told to renegotiate with the payload to get a new data count value. */
		transferControl.state = TRANSFER_INITIALISE;
		/* Clear our renegogotiation flag */
		transferControl.renegotiate = 0;
		retVal = 1;
	}
	return retVal;
}

static uint8_t SetActivePayload(uint8_t z_payload)
{
	uint8_t retVal = 0;

	/* No action unless we see a change. */
	if(z_payload != transferControl.payload)
	{
		/* Clear all data for a new transfer. */
		ResetPayloadDataTransferProtocol();

		/* Record that we have switched payload. */
		retVal = 1;

		/* Update the payload we are addressing. */
		transferControl.payload = z_payload;

		/* Update the expected transmit and receive COB-IDs for the specified payload. */
		switch(z_payload)
		{
			case ACTIVE_PAYLOAD_PCAM:
				transferControl.transmitChannel = COB_ID_PDO_PCAM;
				transferControl.receiveChannel = COB_ID_PDO_PCAM_ACK;
				transferControl.nodeID = NODE_ID_PDO_PCAM;
				break;
			case ACTIVE_PAYLOAD_LMD:
				transferControl.transmitChannel = COB_ID_PDO_LMD;
				transferControl.receiveChannel = COB_ID_PDO_LMD_ACK;
				transferControl.nodeID = NODE_ID_PDO_LMP;
				break;
			case ACTIVE_PAYLOAD_TRI:	
				transferControl.transmitChannel = COB_ID_PDO_TRI;
				transferControl.receiveChannel = COB_ID_PDO_TRI_ACK;
				transferControl.nodeID = NODE_ID_PDO_TRI;
				break;
			case ACTIVE_PAYLOAD_GPS:
				transferControl.transmitChannel = COB_ID_PDO_GPS;
				transferControl.receiveChannel = COB_ID_PDO_GPS_ACK;
				transferControl.nodeID = NODE_ID_PDO_GPS;
				break;
			case ACTIVE_PAYLOAD_OBDH:
				transferControl.transmitChannel = COB_ID_PDO_OBDH;
				transferControl.receiveChannel = COB_ID_PDO_OBDH_ACK;
				transferControl.nodeID = NODE_ID_PDO_OBDH;
				break;
			case ACTIVE_PAYLOAD_SCAM:
				transferControl.transmitChannel = COB_ID_PDO_SCAM;
				transferControl.receiveChannel = COB_ID_PDO_SCAM_ACK;
				transferControl.nodeID = NODE_ID_PDO_SCAM;
				break;
			default:
				break;
		}
	}
	return retVal;
}

static void ResetPayloadDataTransferProtocol(void)
{
	packetsTransferred = 0;
	transferControl.state = TRANSFER_IDLE;
	transferControl.packetsRemaining = 0;
	transferControl.nodeID = 0;
	transferControl.payload = 0;
	transferControl.receiveChannel = 0;
	transferControl.sequenceNumber = 0;
	transferControl.transmitChannel = 0;
	transferControl.readRegister = 1;
	transferControl.renegotiate = 0;
	transferControl.cycleTransfers = 0;
	
	/* Update the packet counter in the CAN OD */
	SetCANObjectDataRaw(AMS_OBC_CAN_PL_PCKTS_REM, transferControl.packetsRemaining);

	if(transferControl.timer != 0)
	{
		/* We need to stop our timer here. */
		if(xTimerStop(transferControl.timer, 20) != pdPASS )
		{
			/* Something has gone very wrong with freertos - deal with it. */
		}
	}
	transferControl.timeOut = 0;
}

uint8_t PayloadDataTransferIsInProgess(void)
{
	return transferControl.state == TRANSFER_IDLE ? 0 : 1;
}

static void TimerExpiredCallback(xTimerHandle xTimer)
{
	transferControl.timeOut = 1;
}

static void ChangeToErrorIfTimeoutHasOccurred(void)
{
	/* If we time out then signal error. */
	if(transferControl.timeOut)
	{
		transferControl.state = TRANSFER_ERROR;
	}
}

uint32_t GetPayloadDataPacketsTransferred(void)
{
	return packetsTransferred;	
}
