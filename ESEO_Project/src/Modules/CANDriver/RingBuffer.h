#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include "stdint.h"
#include "CANDriver.h"

/* Public functions. */
void RingBufferInit(CAN_PACKET * z_memory, uint32_t z_size);
uint8_t RingBufferAddElement(CAN_PACKET * z_value);
CAN_PACKET RingBufferGetElement(void);
uint32_t RingBufferGetCount(void);
uint32_t RingBufferIsFull(void);
uint32_t RingBufferIsEmpty(void);


#endif