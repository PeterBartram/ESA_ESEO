/**
@file 	Diagnostics.c
@author Pete Bartram
@brief 
The intention of this diagnostics software file is to handle calling through into the 
ADC driver to make requests such that the diagnostics data is collected.
All collected data is then passed back up to this software module.
@todo This file is very sparse at the moment and intentionally so, this is because we will need to do data linearisation and interpretation here later.
*/
#include "Diagnostics.h"
#include "ADCDriver.h"

/* RTOS header files. */
#include "FreeRTOS.h"
#include "task.h"
#include "CANODInterface.h"
#include "queue.h"
#include <string.h>
#include "semphr.h"
#include "Watchdog.h"

uint16_t * p_diagnosticData;				/**< Diagnostics data buffer, all retreived ADC data is placed here. */
uint32_t diagnosticDataSize;				/**< The size of the buffer pointed to by p_diagnosticData */
static QueueHandle_t commandQueueHandle;	/**< The handle of the command queue */

static void PushDiagnosticsDataOntoQueue(void);
static void PushSingleTelemetryPointOntoQueue(uint8_t z_canID, uint8_t z_telemetryPoint);

/**
*	This function is the diagnostics task function and should never return.
*/
void DiagnosticsTask(void)
{
	while(1)
	{		
		ProvideThreadHeartbeat(TELEMETRY_THREAD);
		FetchNewDiagnosticData();
		vTaskDelay(5000);
	}
}


/**
*	Perform initalisation of the diagnostics system, this means ensuring that we have a return
*	buffer for data and also that all ADCs could be initialised.	
*	\param[in] z_diagnosticData: Points to the return buffer for sampled data.
*	\param[in] z_size: The size of the return buffer in terms of elements.
*	\return DIAGNOSTICS_ERROR code
*/
DIAGNOSTICS_ERROR InitialiseDiagnostics(uint16_t * z_diagnosticData, uint32_t z_size)
{
	DIAGNOSTICS_ERROR retVal = INITIALISATION_FAILED_DIAG;

	/* Ensure that we have a pointer to return data to and that our size is not zero. */
	if(z_diagnosticData != 0 && z_size != 0)
	{
		/* Store the data return path for later. */
		p_diagnosticData = z_diagnosticData;
		diagnosticDataSize = z_size;

		/* Initialise our ADCs to perform diagnostics. */
		if(ADCInitialise(z_diagnosticData, z_size) == NO_ERROR_ADC)
		{
		    commandQueueHandle =  GetCommandQueueHandle();
			retVal = NO_ERROR_DIAG;
		}
		else
		{
			retVal = INITIALISATION_FAILED_DIAG;
		}
		RegisterThreadWithManager(TELEMETRY_THREAD);
	}
	return retVal;
}

/**
*	This function will go and collect data from all on-board ADC sensors that have been
*	specified in the SENSOR_NAME table.
	\param[out] z_diagnosticData: Data will be returned to the location in p_diagnosticData configured during initialisation.
*	\return DIAGNOSTICS_ERROR code
*/
DIAGNOSTICS_ERROR FetchNewDiagnosticData(void)
{
	DIAGNOSTICS_ERROR retVal = NOT_INITIALISED_DIAG;		
	
	/* Don't attempt to fetch data without having somewhere to put it afterwards. */
	if(p_diagnosticData != 0 && diagnosticDataSize != 0)
	{
		if(FetchDiagnosticsData() == NO_ERROR_ADC)
		{
			PushDiagnosticsDataOntoQueue();
			retVal = NO_ERROR_DIAG;	
		}
		else
		{
			retVal = FETCH_DATA_FAILED_DIAG;
		}		
	}
	else
	{
		retVal = NOT_INITIALISED_DIAG;
	}
	return retVal;
}

/* This function will take all of the current telemetry data and push it
   on the queue to then be placed into the CAN OD. */
static void PushDiagnosticsDataOntoQueue(void)
{	
	if(commandQueueHandle != 0)
	{
		/* Push all of our telemetry points onto the CAN OD queue. */
		PushSingleTelemetryPointOntoQueue(AMS_L_RSSI_TRANS, TRANSPONDER_RSSI);
		PushSingleTelemetryPointOntoQueue(AMS_L_DOPPLER_COMMAND, COMMAND_DOPPLER_SHIFT);
		PushSingleTelemetryPointOntoQueue(AMS_L_RSSI_COMMAND, COMMAND_RSSI);
		PushSingleTelemetryPointOntoQueue(AMS_L_T, COMMAND_OSCILATTOR_TEMPERATURE);
		PushSingleTelemetryPointOntoQueue(AMS_VHF_BPSK_PA_T, BPSK_POWER_AMPLIFIER_TEMPERATURE);
		PushSingleTelemetryPointOntoQueue(AMS_VHF_FP, BPSK_FORWARD_POWER);
		PushSingleTelemetryPointOntoQueue(AMS_VHF_RP, BPSK_REVERSE_POWER);
		PushSingleTelemetryPointOntoQueue(AMS_VHF_BPSK_PA_I, BPSK_POWER_AMPLIFIER_CURRENT);
		PushSingleTelemetryPointOntoQueue(AMS_VHF_FM_PA_I, FM_POWER_AMPLIFIER_CURRENT);
		PushSingleTelemetryPointOntoQueue(AMS_VHF_FM_PA_T, FM_POWER_AMPLIFIER_TEMPERATURE);
		PushSingleTelemetryPointOntoQueue(AMS_VHF_BPSK_I, BPSK_TONE_CURRENT);
		PushSingleTelemetryPointOntoQueue(AMS_EPS_CCT_T, EPS_PROCESSOR_TEMPERATURE);
		PushSingleTelemetryPointOntoQueue(AMS_EPS_ENC_T, STRUCTURE_TEMPERATURE);
		PushSingleTelemetryPointOntoQueue(AMS_EPS_DCDC_T, EPS_DC_TO_DC_CONVERTER_TEMPERATURE);
		PushSingleTelemetryPointOntoQueue(AMS_EPS_DCDC_I, EPS_DC_TO_DC_CONVERTER_CURRENT);
		PushSingleTelemetryPointOntoQueue(AMS_EPS_DCDC_V, EPS_DC_TO_DC_CONVERTER_VOLTAGE);
		PushSingleTelemetryPointOntoQueue(AMS_EPS_6V_V, EPS_SMPS_6V_VOLTAGE);
		PushSingleTelemetryPointOntoQueue(AMS_EPS_9V_V, EPS_SMPS_9V_VOLTAGE);
		PushSingleTelemetryPointOntoQueue(AMS_EPS_3V3_V, EPS_SMPS_3V_VOLTAGE);
		PushSingleTelemetryPointOntoQueue(AMS_EPS_6V_I, EPS_SMPS_6V_CURRENT);
		PushSingleTelemetryPointOntoQueue(AMS_EPS_3V3_I, EPS_SMPS_3V_CURRENT);
		PushSingleTelemetryPointOntoQueue(AMS_EPS_9V_I, EPS_SMPS_9V_CURRENT);
	}
	else
	{
		/* Error handler should do something here. */
	}
}

/* Take a single telemetry data point, format it correctly and place onto the command queue. */
static void PushSingleTelemetryPointOntoQueue(uint8_t z_canID, uint8_t z_telemetryPoint)
{
	uint8_t buffer[CAN_QUEUE_ITEM_SIZE];
	memset(buffer, 0, sizeof(buffer));
	/* Indicate where this packet has come from. */
	buffer[PACKET_SOURCE_INDEX] = ON_TARGET_DATA;
	/* Standard packet size for diagnostics data */
	buffer[PACKET_SIZE_INDEX] = 5;
	/* Add the can id we are sending. */
	buffer[OBJECT_DICTIONARY_ID_INDEX] = z_canID;
	/* We need to lose the two LSBs and replace with the two MSBs from the second byte. */
	buffer[PACKET_DATA_INDEX + 3] = (p_diagnosticData[z_telemetryPoint] >> 2);	// + 3 because of endianism when pushing into the CAN-OD.
	xQueueSend(commandQueueHandle, buffer, 0);
}

/**
*	Wipe down the return buffer pointer, no action needs to be taken inside the ADC driver.
*	\return DIAGNOSTICS_ERROR code
*/
void UninitialiseDiagnostics(void)
{
	p_diagnosticData = 0;
	diagnosticDataSize = 0;
	ADCUninitialise();
}
