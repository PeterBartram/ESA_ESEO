#ifndef DOWNLINK_H_
#define DOWNLINK_H_

#include <stdint.h>

typedef enum _DOWNLINK_ERROR
{
	NO_ERROR_DOWNLINK = 0
}DOWNLINK_ERROR;

/* Which of the RF modes are we currently in? */
typedef enum _DATA_MODE
{
	MODE_RX_ONLY = 0,		// Receive only mode
	MODE_1K2 = 1,			// 1k2 telemetry mode
	MODE_4K8 = 2,			// 4k8 telemtry mode
	MODE_4K8_PAYLOAD = 3	// 4k8 telemetry + payload data mode
}DATA_MODE;

void InitialiseDownlink(void);
void DownlinkThread(void);
void DownlinkTask(void);
void ChangeDownlinkMode(DATA_MODE z_mode);

#endif /* DOWNLINK_H_ */