#include <stdint.h>
#include <stdio.h>
#include <tchar.h>

#include "gtest/gtest.h"
extern "C"
{
	#include "CANDriver.c"
}

volatile can_msg_t *p_messageObjectBufferChannel0 = messageObjectBuffer;	
uint32_t * p_messageObjectAllocationTable = &messageObjectAllocationTable;

void((*p_CANTransmitOK_ISR)(void)) = CANTransmitOK_ISR;
void((*p_CANReceiveOK_ISR)(void)) = CANReceiveOK_ISR;
void((*p_CANBusOff_ISR)(void)) = CANBusOff_ISR;
void((*p_CANError_ISR)(void)) = CANError_ISR;
void((*p_CANWakeUp_ISR)(void)) = CANWakeUp_ISR;

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}