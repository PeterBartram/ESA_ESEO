#ifndef CANOPEN_H_
#define CANOPEN_H_

#include "CANDriver.h"
#include "CANObjectDictionary.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Node ID */
#define PAYLOAD_NODE_ID 102

/* Permanent defines - don't change these! */
#define COB_ID_PDO_GET				0x1B5
#define COB_ID_PDO_SET				0x1B4
#define COB_ID_PDO_SETT				0x190
#define COB_ID_PDO_ACK				0x1B6
#define COB_ID_PDO_PCAM				0x1CB
#define COB_ID_PDO_PCAM_ACK			0x1CC
#define COB_ID_PDO_TRI				0x1CD
#define COB_ID_PDO_TRI_ACK			0x1CE
#define COB_ID_PDO_LMD				0x1CF
#define COB_ID_PDO_LMD_ACK			0x1D0
#define COB_ID_PDO_GPS				0x1D1
#define COB_ID_PDO_GPS_ACK			0x1D2
#define COB_ID_PDO_OBDH				0x1E5
#define COB_ID_PDO_OBDH_ACK			0x1E6
#define COB_ID_PDO_SCAM				0x1DC
#define COB_ID_PDO_SCAM_ACK			0x1DE
#define COB_ID_SDO_OBDH				0x672
#define COB_ID_SDO_OBDH_ACK			0x5F2
#define COB_ID_HEARTBEAT_CONSUMER	0x701
#define COB_ID_HEARTBEAT_PRODUCER   (0x700 + PAYLOAD_NODE_ID)

/* NODE IDs for payloads. */
#define NODE_ID_PDO_PCAM		102	
#define NODE_ID_PDO_TRI			104
#define NODE_ID_PDO_LMP			105			
#define NODE_ID_PDO_GPS			106
#define NODE_ID_PDO_OBDH		1
#define NODE_ID_PDO_SCAM		107

typedef enum 
{
	NO_ERROR_CAN_OPEN = 0,
	NULL_POINTER_CAN_OPEN,
	INVALID_PDO_PACKET,
	INITIALISATION_FAILED_CAN_OPEN,
	COULD_NOT_EXECUTE_ACTION
}CAN_OPEN_ERROR;

typedef enum
{
	CAN_BUS_ACTIVE = 0,
	CAN_BUS_INACTIVE,
	CAN_BUS_SWITCHING
}NETWORK_STATE;

/* Interface functions */
CAN_OPEN_ERROR InitialiseCANOpen(QueueHandle_t z_payloadDataQueue);
void CANOpenTask(void);
NETWORK_STATE GetNetworkState(void);

#endif