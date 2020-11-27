#ifndef SATELLITEOPERATIONS_H_
#define SATELLITEOPERATIONS_H_

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdint.h>
#include <string.h>
#include "timers.h"
#include "BitStream.h"
#include "CANObjectDictionary.h"
#include "CANODInterface.h"
#include <math.h>

/* Configuration values - don't change these! */
#define AUTO_RELOAD						1

void InitialiseSatelliteOperations(SemaphoreHandle_t z_flashSemaphore);
void SatelliteOperationsThread(void);
void SatelliteOperationsTask(void);
uint32_t GetWODData(uint8_t z_wodDataNumber, uint8_t * z_wodDataBuffer);


#endif /* SATELLITEOPERATIONS_H_ */