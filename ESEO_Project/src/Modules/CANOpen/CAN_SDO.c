#include "CAN_SDO.h"

/* Permanent defines - dont change any of these! */
#define SDO_PACKET_SIZE 8
#define PACKET_DATA_SIZE 7
#define COMMAND_PACKET_INDEX 0
#define COMMAND_DATA_SIZE 1
#define DATA_IN_PACKET_FIELD 1
#define TRANSFER_COMPLETE_BIT 0
#define TOGGLE_BIT 4

#define COMMAND_BIT_SHIFT_COUNT 5
#define COMMAND_SEGMENT 0
#define COMMAND_INITIATE 2
#define COMMAND_TRANSFER 3
#define COMMAND_ABORT 4

#define ADDRESS_LSB_INDEX 1
#define ADDRESS_MSB_INDEX 2
#define ADDRESS_SUB_INDEX 3

/* Local variables for this module - This .c file is being 
treated as an object for the purposes of a transfer. */
static SDO_TRANSFER_STATE state;
static uint8_t housekeepingDataCount;
static uint8_t housekeepingDataSent;
static uint8_t * housekeepingData;
static uint8_t toggle;

/* Private functions */
static void ResetServiceDataObject(void);
static void AbortTransmission(void);
static void AcceptTransmission(void);
static void SendSegmentData(void);
static uint8_t PacketAddressIsValid(CAN_PACKET * z_packet);
static void PerformIdleTransferOperations(CAN_PACKET * z_packet);
static void PerformActiveTransferOperations(CAN_PACKET * z_packet);

/**
*	Perform initalisation of the SDO transfer protocol.
*/
void InitialiseServiceDataObject(void)
{
	ResetServiceDataObject();
}

/**
*	This is the main update method for actioning a transfer and should be called when packets are recevied.
*	\param[in] z_packet - the packet recevied over the appropriate SDO channel.
*/
void SDOHousekeepingUpdate(CAN_PACKET * z_packet)
{
	if(z_packet)
	{
		/* This is our main loop for actioning the packet received based upon the current state. */
		switch(state)
		{
			case SDO_IDLE:
				PerformIdleTransferOperations(z_packet);
				break;
			case SDO_ACTIVE:
				PerformActiveTransferOperations(z_packet);
				break;
		}
	}
}

/**
*	Returns the current transfer state of the system.
*	\return SDO_IDLE or SDO_ACTIVE
*/
SDO_TRANSFER_STATE GetTransferState(void)
{
	return state;
}

/**
*	Perform transfer operations when the system is in idle mode, essentially looking for a 
*	transfer to begin and then getting a copy of the hosuekeeping data.
*/
static void PerformIdleTransferOperations(CAN_PACKET * z_packet)
{
	uint8_t command = (z_packet->data[COMMAND_PACKET_INDEX] >> COMMAND_BIT_SHIFT_COUNT);
	if(command == COMMAND_INITIATE)
	{
		if(PacketAddressIsValid(z_packet))
		{
			housekeepingDataCount = GetHousekeepingData(&housekeepingData);
			AcceptTransmission();
			state = SDO_ACTIVE;
		}
	}
	else if(command == COMMAND_ABORT)
	{
		ResetServiceDataObject();
	}
	else
	{
		ResetServiceDataObject();
		AbortTransmission();
	}
}

/**
*	Perform transfer operations when the system is in active mode, essentially looking for a 
*	data request packet and then replying to this with the required data.
*/
static void PerformActiveTransferOperations(CAN_PACKET * z_packet)
{
	/* Extract the SDO command from our packet. */
	uint8_t command = (z_packet->data[COMMAND_PACKET_INDEX] >> COMMAND_BIT_SHIFT_COUNT);

	if(command == COMMAND_TRANSFER)
	{
		SendSegmentData();
	}
	else if(command == COMMAND_ABORT)
	{
		ResetServiceDataObject();
	}
	else
	{
		ResetServiceDataObject();
		AbortTransmission();
	}
}

/**
*	Send the next snippet of data requested.
*/
static void SendSegmentData(void)
{
	CAN_PACKET outgoingPacket = {0};
	uint8_t i;

	/* Configure basic fixed requirements of the reply packet. */
	outgoingPacket.dataSize = SDO_PACKET_SIZE;
	outgoingPacket.id = 0x5F2;
	outgoingPacket.data[COMMAND_PACKET_INDEX] = (COMMAND_SEGMENT << COMMAND_BIT_SHIFT_COUNT);

	/* It is necessary for us to toggle a bit each time we send a packet, this is to comply
	   with the CanOpen segmented transfer protocol. */
	if(toggle)
	{
		/* Set the appropriate bit in the response packet. */
		outgoingPacket.data[COMMAND_PACKET_INDEX] |= (0x01 << TOGGLE_BIT);
	}
	toggle = !toggle;

	/* Should always be true but just to ensure no buffer overflows */
	if(housekeepingDataSent < housekeepingDataCount)
	{
		/* For each free data slot in our packet. */
		for(i = 0; i < PACKET_DATA_SIZE; i++)
		{
			/* Check if we have finished the packet. */
			if((housekeepingDataSent + i) < housekeepingDataCount)
			{
				/* Update the data fields in our packet. */
				outgoingPacket.data[i + COMMAND_DATA_SIZE] = housekeepingData[housekeepingDataSent + i];
			}
			else
			{
				break;
			}
		}

		/* Update our count of how much data we have sent. */
		housekeepingDataSent += i;

		/* If we have added all housekeeping data. */
		if(housekeepingDataSent == housekeepingDataCount)
		{
			/* We need to calculate how many bytes do not contain data in our packet and update accordingly. */
			uint8_t remaining = PACKET_DATA_SIZE - i;
			outgoingPacket.data[COMMAND_PACKET_INDEX] |= (remaining << DATA_IN_PACKET_FIELD);
			/* We also need to mark this transfer as complete. */
			outgoingPacket.data[COMMAND_PACKET_INDEX] |= (0x01 << TRANSFER_COMPLETE_BIT);
			ResetServiceDataObject();
		}
	}
	TransmitCAN(outgoingPacket);	
	
}

/**
*	Send a response to a transmission begin request accepting the transmission.
*/
static void AcceptTransmission(void)
{
	CAN_PACKET outgoingPacket = {0};
	outgoingPacket.dataSize = SDO_PACKET_SIZE;
	outgoingPacket.id = 0x5F2;
	outgoingPacket.data[COMMAND_PACKET_INDEX] = (COMMAND_INITIATE << COMMAND_BIT_SHIFT_COUNT);
	outgoingPacket.data[1] = 0x0;
	outgoingPacket.data[2] = 0x20;
	outgoingPacket.data[3] = 0x0;
	TransmitCAN(outgoingPacket);
}

/**
*	Send an abort packet.
*/
static void AbortTransmission(void)
{
	CAN_PACKET outgoingPacket = {0};
	outgoingPacket.dataSize = SDO_PACKET_SIZE;
	outgoingPacket.id = COB_ID_SDO_OBDH_ACK;
	outgoingPacket.data[COMMAND_PACKET_INDEX] = (COMMAND_ABORT << COMMAND_BIT_SHIFT_COUNT);
	outgoingPacket.data[1] = 0x0;
	outgoingPacket.data[2] = 0x20;
	outgoingPacket.data[3] = 0x0;
	TransmitCAN(outgoingPacket);
}

/**
*	Reset all of our state variables.
*/
static void ResetServiceDataObject(void)
{
	state = SDO_IDLE;
	housekeepingData = 0;
	housekeepingDataCount = 0;
	housekeepingDataSent = 0;
	toggle = 0;
}

/**
*	Check that our singular address of 0x2000 with subindex 0x00 is correct.
*/
static uint8_t PacketAddressIsValid(CAN_PACKET * z_packet)
{
	if(z_packet->data[ADDRESS_LSB_INDEX] == 0x00 &&	z_packet->data[ADDRESS_MSB_INDEX] == 0x20 && z_packet->data[ADDRESS_SUB_INDEX] == 0x00)
	{
		return 1;
	}
	else
	{
		return 0;
	}	
}