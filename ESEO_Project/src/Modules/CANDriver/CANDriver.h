/**
@file 	CANDriver.h
@author Pete Bartram
@brief 
*/
#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include "canif.h"

/** Possible CAN frames that can be sent. */
typedef enum _CAN_REQUEST_TYPE
{
	DATA_FRAME = 0,
	REMOTE_FRAME
}CAN_REQUEST_TYPE;

/** Contains a CAN packet to transmit or receive. */
typedef struct _CAN_PACKET
{
	uint16_t id;					/**< CAN ID */
	CAN_REQUEST_TYPE requestType;	/**< The type of frame */
	uint8_t dataSize;				/**< Size of the data to be transmitted or received - max = 8bytes */
	uint8_t data[8];				/**< Data to be transmitted / received. */
}CAN_PACKET;

/** CAN errors. */
typedef enum _CAN_ERROR
{
	NO_ERROR_CAN = 0,
	ERROR_DATA_LENGTH_TOO_LONG,
	ERROR_MOB_NOT_AVALIABLE,
	ERROR_CHANNEL_NUMBER,
	ERROR_INVALID_MOB_HANDLE,
	NULL_POINTER,
	NO_PACKETS_RECEIVED,
	INITIALISATION_FAILED,
	TOO_MANY_RECEVIE_COB_IDS
}CAN_ERROR;

/* Core interface functions. */
CAN_ERROR InitialiseCANDriver(uint16_t * z_receiveCODIDs, uint8_t z_numberOfReceiveCOBIDs);
CAN_ERROR TransmitCAN(CAN_PACKET z_packet);
CAN_ERROR ReceiveCAN(CAN_PACKET * z_packet);
CAN_ERROR ChangeCANChannel(void);
uint8_t CANHasTransmitSpaceAvailable(void);

/* Diagnostics interface functions */
uint32_t PacketsDropped(void);
void ResetPacketsDroppedCount(void);
uint64_t PacketsReceived(void);
void ResetPacketsReceivedCount(void);
void ResetFailedTransmission(void);
uint64_t FailedTransmissions(void);
void ResetPacketsTransmittedCount(void);
uint32_t PacketsTransmitted(void);
	
#endif
