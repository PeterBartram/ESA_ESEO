#ifndef NMT_H_
#define NMT_H_

#include "CANOpen.h"

typedef enum
{
	BUS_ACTIVE = 0,
	BUS_INACTIVE,
	BUS_SWITCHING
}NMT_STATE;

CAN_OPEN_ERROR InitialiseNMT(void);
void UpdateNetworkManagement(void);
void HeartBeatPacketReceived(CAN_PACKET * z_incomingPacket);
NMT_STATE GetNMTState(void);

#endif 