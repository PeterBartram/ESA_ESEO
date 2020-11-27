//headers
#include <asf.h>
#include <string.h>
#include "modulator.h"
#include "DACout.h"
#include "FECencoder.h"
#include "AX25encoder.h"
#include "FreeRTOS.h"
#include "task.h"

#define DELAY_1_MS (1/portTICK_RATE_MS)

/* Fixed length packet due to FEC - DO NOT CHANGE */
#define MOD_PACKET_SIZE	256	

// sample data to be encoded
int modBusy = 0;
uint8_t modWaitBuff[MOD_PACKET_SIZE];
static uint8_t i = 0;	// for testing.

//initialize modulator
void InitModulator(void)
{
	//initialize FEC and DAC
	InitFEC();
	InitialiseDAC();
	
	return;
}

uint16_t tempBuffer[MOD_PACKET_SIZE];	// Lose this later.
void ModulatorThread(void)
{
	while(1)
	{		
		TasksModulator();
		vTaskDelay(10);
	}
}

void TasksModulator(void)
{
	uint16_t count;
	uint8_t * outputBuffer = 0;
	
	/* Dont try to transmit if we dont have a buffer to output to or don't have any input data. */
	if(modBusy && (DACoutBuffSize() == 0)) //only send with empty DAC buffer
	{		
		/* Perform the FEC */
		outputBuffer = EncodeFEC( modWaitBuff, &count);	
		/* Tell the DAC that its services are required. */
		DACOutputBufferReady (outputBuffer, count);
		/* Allow more data to be added to the input buffer. */
		modBusy = 0;
	}
	
	return;
}


// send some packet
int SendModulatorPacket( uint8_t *packet )
{
	if( !modBusy )	//if mod is not busy, set it to busy and copy the packet into modwaitBUff
	{
		modBusy = 1;
		memcpy( modWaitBuff, packet, sizeof(modWaitBuff) );
		
		return 1;	//Success, packet loaded into modwaitBuff
	}
	
	return 0; // modulator busy, wait
}

/*
	Change the modulation mode (Low power / High Power and 1K2 / 4K8)
*/
void ModulatorChangeMode(TRANSMISSION_MODE z_mode)
{
	DACChangeModulatorMode(z_mode);
}