#include "Fake_CANDriver.h"

DEFINE_FAKE_VALUE_FUNC1(CAN_ERROR, ReceiveCAN, CAN_PACKET*);
DEFINE_FAKE_VALUE_FUNC1(CAN_ERROR, TransmitCAN, CAN_PACKET);
DEFINE_FAKE_VALUE_FUNC0(uint8_t, CANHasTransmitSpaceAvailable);
DEFINE_FAKE_VALUE_FUNC2(CAN_ERROR, InitialiseCANDriver, uint16_t *, uint8_t);
DEFINE_FAKE_VALUE_FUNC0(CAN_ERROR, ChangeCANChannel);

void ResetPacketsDroppedCount(void){}
void ResetPacketsReceivedCount(void){}
void ResetFailedTransmission(void){}
void ResetPacketsTransmittedCount(void){}
uint32_t PacketsTransmitted(void){return 1;}
uint64_t PacketsReceived(void) {return 1;}