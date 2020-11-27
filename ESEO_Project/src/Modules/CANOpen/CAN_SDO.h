#ifndef CAN_SDO_H_
#define CAN_SDO_H_

#include "CANOpen.h"

/* The possible states of our transfer process. */
typedef enum
{
	SDO_IDLE = 0,
	SDO_ACTIVE
}SDO_TRANSFER_STATE;

/* Interface functions */
void InitialiseServiceDataObject(void);
void SDOHousekeepingUpdate(CAN_PACKET * z_packet);
SDO_TRANSFER_STATE GetTransferState(void);

#endif