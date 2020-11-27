#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "NMT.h"
#include "CANDriver.h"

#define DELAY_1000MS  (1000/portTICK_RATE_MS)
#define TIMER_RESET_TIMEOUT_MS 20
#define SKIPPED_BEATS_FOR_BUS_CHANGE_COUNT 20
#define HEARTBEAT_CONTENT 5

static xTimerHandle heartbeatConsumerTimer = 0;
static StaticTimer_t canOpenHeartBeatTimer;
static uint8_t heartBeatTimeoutOccurred = 0;
static uint8_t skippedBeats = 0;
static uint8_t busActive = 0;
static NMT_STATE networkState = BUS_INACTIVE;

/* Private function prototypes */
static void HeartBeatConsumerTimeout(xTimerHandle xTimer);

CAN_OPEN_ERROR InitialiseNMT(void)
{
	/* Ensure we dont start to broadcast until we have heard a heartbeat. */
	networkState = BUS_INACTIVE;

	heartbeatConsumerTimer = xTimerCreateStatic("HeartbeatConsumerTimer", DELAY_1000MS, 0, 0, &HeartBeatConsumerTimeout, &canOpenHeartBeatTimer);

	if(heartbeatConsumerTimer != 0)
	{
		if(xTimerReset(heartbeatConsumerTimer, TIMER_RESET_TIMEOUT_MS) != pdPASS)
		{
			/** @todo deal with this! */
		}
	}
	else
	{
		/** @todo deal with this! */
	}

	return heartbeatConsumerTimer ? NO_ERROR_CAN_OPEN : INITIALISATION_FAILED_CAN_OPEN;
}

void UpdateNetworkManagement(void)
{
	if(heartBeatTimeoutOccurred)
	{
		/* Record that we have missed a beat */
		skippedBeats++;

		/* Reset our timeout flag */
		heartBeatTimeoutOccurred = 0;

		/* If we have missed enough beats to warrant changing channel. */
		if(skippedBeats >= SKIPPED_BEATS_FOR_BUS_CHANGE_COUNT)
		{
			ChangeCANChannel();
			skippedBeats = 0;
			networkState = BUS_SWITCHING;
		}

		/* If we are switching bus then we need to broadcast our heartbeat here. */
		if(networkState == BUS_SWITCHING)
		{
			CAN_PACKET outgoingPacket;
		
			outgoingPacket.data[0] = HEARTBEAT_CONTENT;
			outgoingPacket.dataSize = 1;
			outgoingPacket.id = COB_ID_HEARTBEAT_PRODUCER;
			outgoingPacket.requestType = DATA_FRAME;
		
			TransmitCAN(outgoingPacket);
		}
	
		if(heartbeatConsumerTimer != 0)
		{
			xTimerReset(heartbeatConsumerTimer, TIMER_RESET_TIMEOUT_MS);
		}
	}
}

void HeartBeatPacketReceived(CAN_PACKET * z_incomingPacket)
{
	if(z_incomingPacket)
	{
		/* Record that we have not missed any beats so far. */
		skippedBeats = 0;

		/* Have heard a heartbeat - mark the bus as active. */
		networkState = BUS_ACTIVE;

		/* Reset our timer for the next heartbeat */
		if(heartbeatConsumerTimer != 0)
		{
			if(xTimerReset(heartbeatConsumerTimer, TIMER_RESET_TIMEOUT_MS) != pdPASS)
			{
				/** Houston we have a problem. @todo deal with this! */
			}
		}
	}
}

NMT_STATE GetNMTState(void)
{
	return networkState;
}

static void HeartBeatConsumerTimeout(xTimerHandle xTimer)
{
	heartBeatTimeoutOccurred = 1;
}

