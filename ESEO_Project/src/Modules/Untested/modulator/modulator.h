#ifndef	__MODULATOR_H__
#define	__MODULATOR_H__

#include <stdint.h>

typedef enum _TRANSMISSION_MODE
{
	MODE_1K2_LOW_POWER,
	MODE_1K2_HIGH_POWER,
	MODE_4K8_LOW_POWER,
	MODE_4K8_HIGH_POWER,
}TRANSMISSION_MODE;

//public function prototypes
void InitModulator(void);
void ModulatorThread(void);
void TasksModulator(void);
int SendModulatorPacket(uint8_t *packet );
void ModulatorChangeMode(TRANSMISSION_MODE z_mode);


#endif	//__MODULATOR_H__