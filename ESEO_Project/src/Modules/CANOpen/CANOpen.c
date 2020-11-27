#include "FreeRTOS.h"
#include "task.h"
#include "CANOpen.h"
#include "CAN_PDO.h"
#include "CAN_SDO.h"
#include "NMT.h"
#include "CANQueue.h"
#include "CANODInterface.h"
#include "timers.h"
#include "Watchdog.h"

#define DELAY_1_MS (1/portTICK_RATE_MS)
#define DELAY_10_MS (DELAY_1_MS * 10)
#define DELAY_100_MS (DELAY_1_MS * 100)

#define CAN_PACKETS_TO_PROCESS	4	// Set to four as we only have four outputs.

/* These are the COB-IDs that we are going to listen on */
static uint16_t cobIDs[12] = 
{
	0x666, 
	COB_ID_SDO_OBDH, 
	0x190, 
	COB_ID_PDO_SET,	
	COB_ID_PDO_GET,	
	COB_ID_PDO_PCAM_ACK,		
	COB_ID_PDO_TRI_ACK,		
	COB_ID_PDO_LMD_ACK,	
	COB_ID_PDO_GPS_ACK,	
	COB_ID_PDO_OBDH_ACK,	
	COB_ID_PDO_SCAM_ACK,
	COB_ID_HEARTBEAT_CONSUMER
};

/* Private function prototypes */
static void CANOpenUpdate(void);
static void ServiceIncomingPackets(void);
static void UpdateTransferProtocols(void);
static void CheckCommandQueue(void);
static void UpdateCANPacketCounters(void);
static StaticTimer_t packetCounterUpdateTimer;
static xTimerHandle  packetCounterUpdateTimerHandle;
#define DELAY_1S 1000
#define AUTO_RELOAD 1

static void packetCounterUpdateTimerCallback(xTimerHandle xTimer);
static uint8_t packetCounterUpdate = 0;

CAN_OPEN_ERROR InitialiseCANOpen(QueueHandle_t z_payloadDataQueue)
{
	/* Assume failure for now. */
	CAN_OPEN_ERROR retVal = INITIALISATION_FAILED_CAN_OPEN;

	/* Initialise the CAN driver */
	if(InitialiseCANDriver(cobIDs, (sizeof(cobIDs)/sizeof(cobIDs[0]))) == NO_ERROR_CAN)
	{
		/* Call into PDO to make any PDO specific initialisations. */
		if(InitialiseProcessDataObjects(z_payloadDataQueue) == NO_ERROR_CAN_OPEN)
		{
			if(InitialiseNMT() == NO_ERROR_CAN_OPEN)
			{
				retVal = NO_ERROR_CAN_OPEN;	
			}
		}
	}
	
	/* We want to do this even if the CAN bus wont start */
	InitialiseCanObjectDictionary();
	InitialiseCANQueue();
	
	RegisterThreadWithManager(CAN_OPEN_THREAD);
	
	packetCounterUpdateTimerHandle = xTimerCreateStatic("CANPacketUpdate", DELAY_1S, AUTO_RELOAD, 0, &packetCounterUpdateTimerCallback, &packetCounterUpdateTimer);
	xTimerStart(packetCounterUpdateTimerHandle, 0);
	return retVal;
}

void CANOpenTask(void)
{
	uint8_t i = 0;
	
	while(1)
	{
		ProvideThreadHeartbeat(CAN_OPEN_THREAD);
		for(i = 0; i < CAN_PACKETS_TO_PROCESS; i++)
		{
			CANOpenUpdate();	
		}
		vTaskDelay(1);
	}
}


static void CANOpenUpdate(void)
{
	uint8_t i = 0;
	
	/* Update our network management */
	UpdateNetworkManagement();

	/* Check for any received packets to be dealt with. */
	ServiceIncomingPackets();
	
	/* Update any data transfer protocols. */
	UpdateTransferProtocols();
	
	/* Deal with any packets from the rest of the system and the uplink */
	ServiceCommandQueue();
	
	/* Update the packet counters in the CAN OD */
	UpdateCANPacketCounters();
}

static void UpdateTransferProtocols(void)
{
	/* If we don't have a slot available to reply then we do not attempt any actions. */
	if(CANHasTransmitSpaceAvailable())	///@todo necessary to record when this happens to make sure no problems with our driver.
	{
		/* Perform an update on the payload data transfer protocol */
		CANPDOPayloadTransferProtocolUpdate();
	}
}

NETWORK_STATE GetNetworkState(void)
{
	/* Just map the NMT enum to the CANOpen enum. */
	switch(GetNMTState())
	{
		case BUS_ACTIVE: return CAN_BUS_ACTIVE;
		case BUS_INACTIVE: return CAN_BUS_INACTIVE;
		case BUS_SWITCHING: return CAN_BUS_SWITCHING;
	}
}

static void ServiceIncomingPackets(void)
{
	CAN_PACKET	incomingPacket;
	CAN_PACKET	outgoingPacket;
	/* If we don't have a slot available to reply then we do not attempt any actions. */
	if(CANHasTransmitSpaceAvailable())	///@todo necessary to record when this happens to make sure no problems with our driver.
	{
		/* Remove a packet from our CAN buffer if there is one. */
		if(ReceiveCAN(&incomingPacket) == NO_ERROR_CAN)
		{
			/* Switch statement based upon connection object ID received. */
			switch(incomingPacket.id)
			{
				case COB_ID_SDO_OBDH:
					SDOHousekeepingUpdate(&incomingPacket);				
					break;
				case COB_ID_PDO_GET:
					if(ProcessDataObjectGET(&incomingPacket, &outgoingPacket) == NO_ERROR_CAN_OPEN)
					{
						TransmitCAN(outgoingPacket);
					}
					break;
				case COB_ID_PDO_SET:
					if(ProcessDataObjectSET(&incomingPacket, &outgoingPacket) == NO_ERROR_CAN_OPEN)
					{
						TransmitCAN(outgoingPacket);
					}
					break;
				case COB_ID_PDO_SETT:
					ProcessDataObjectSETT(&incomingPacket);
					break;
				case COB_ID_PDO_PCAM_ACK:
					ReceivedPayloadDataTransferProtocolAcknowledge(&incomingPacket);
					break;
				case COB_ID_PDO_TRI_ACK:
					ReceivedPayloadDataTransferProtocolAcknowledge(&incomingPacket);
					break;
				case COB_ID_PDO_LMD_ACK:
					ReceivedPayloadDataTransferProtocolAcknowledge(&incomingPacket);
					break;
				case COB_ID_PDO_GPS_ACK:
					ReceivedPayloadDataTransferProtocolAcknowledge(&incomingPacket);
					break;
				case COB_ID_PDO_OBDH_ACK:
					ReceivedPayloadDataTransferProtocolAcknowledge(&incomingPacket);
					break;
				case COB_ID_PDO_SCAM_ACK:
					ReceivedPayloadDataTransferProtocolAcknowledge(&incomingPacket);
					break;
				case COB_ID_HEARTBEAT_CONSUMER:
					HeartBeatPacketReceived(&incomingPacket);
					break;
				default:
					break;
			}
		}
	}
}

static void UpdateCANPacketCounters(void)
{
	/* Update all counter variables */
	if(packetCounterUpdate != 0)
	{
		uint32_t canTransactions = 0;
		uint32_t canPayloadTransactions;
		packetCounterUpdate = 0;
		canTransactions = PacketsReceived() + PacketsTransmitted();
		canPayloadTransactions = GetPayloadDataPacketsTransferred();
		SetCANObjectDataRaw(AMS_OBC_CAN_STATUS, canTransactions);
		SetCANObjectDataRaw(AMS_OBC_CAN_PL_STATUS, canPayloadTransactions);
	}
}

static void packetCounterUpdateTimerCallback(xTimerHandle xTimer)
{
	packetCounterUpdate = 1;
}