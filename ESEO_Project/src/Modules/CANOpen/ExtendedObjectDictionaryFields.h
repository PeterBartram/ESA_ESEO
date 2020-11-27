#ifndef EXTENDEDOBJECTDICTIONARYFIELDS_H_
#define EXTENDEDOBJECTDICTIONARYFIELDS_H_

#include <stdint.h>
#include "CANOpen.h"
#include "semphr.h"

CAN_OPEN_ERROR SetHolemapField(uint8_t z_id, uint32_t z_data, uint8_t z_changed);
CAN_OPEN_ERROR GetHolemapField(uint8_t z_id, uint32_t * z_data);
CAN_OPEN_ERROR SetFitterMessageField(uint8_t z_id, uint32_t z_data, uint8_t z_changed);
CAN_OPEN_ERROR GetFitterMessageField(uint8_t z_id, uint32_t * z_data);
CAN_OPEN_ERROR ChangeFitterMessageSlot(uint8_t z_id, uint32_t z_data, uint8_t z_changed);
void InitialiseExtendedObjectFields(SemaphoreHandle_t z_canObjectDictionarySemaphore);

#endif