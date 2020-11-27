#ifndef _FAKE_CAN_OBJECT_DICTIONARY_H
#define _FAKE_CAN_OBJECT_DICTIONARY_H

#include "CANObjectDictionary.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC2(OBJECT_DICTIONARY_ERROR, GetCANObjectData, uint32_t, uint32_t *);
DECLARE_FAKE_VALUE_FUNC2(OBJECT_DICTIONARY_ERROR, SetCANObjectData, uint32_t, uint32_t);
DECLARE_FAKE_VALUE_FUNC2(OBJECT_DICTIONARY_ERROR, GetCANObjectDataRaw, uint32_t, uint32_t *);
DECLARE_FAKE_VALUE_FUNC2(OBJECT_DICTIONARY_ERROR, SetCANObjectDataRaw, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(SettCANObjectData, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC1(PerformEventActions, uint8_t);
DECLARE_FAKE_VALUE_FUNC1(uint8_t, GetHousekeepingData, uint8_t **);
DECLARE_FAKE_VALUE_FUNC1(OD_DATA_TYPE, GetFieldType, uint8_t);
DECLARE_FAKE_VOID_FUNC0(InitialiseCanObjectDictionary);
DECLARE_FAKE_VOID_FUNC3(GetFitterMessage, uint8_t, uint8_t *, uint16_t);
DECLARE_FAKE_VOID_FUNC0(ClearHoleMap);
DECLARE_FAKE_VOID_FUNC2(GetHolemap, uint8_t *, uint16_t);

#endif