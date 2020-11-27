/**
@file 	Sensors.h
@author Pete Bartram
@brief 
The intention of this diagnostics software file is to handle looping through a list of diagnostics / telemetry
sensors and then calling through into the ADC driver to make requests such that the diagnostics data is collected.
All collected data is then passed back up to the calling function.
@todo The data here really needs to be linearized/interpreted such that a representative value can be returned rather
than just the raw data.
*/

#ifndef SENSORS_H_
#define SENSORS_H_

/** 
Contains the list of sensors that are to provide telemetry data.
The number of sensors in this list MUST match up with ADCMap[] in ADCMapping.h
*/
typedef enum _SENSOR_NAME
{
	/* LBAND Board */
	TRANSPONDER_RSSI = 0,
	COMMAND_DOPPLER_SHIFT,
	COMMAND_RSSI,
	COMMAND_OSCILATTOR_TEMPERATURE,
	/* BPSK Board */
	BPSK_REVERSE_POWER,
	BPSK_FORWARD_POWER,
	BPSK_POWER_AMPLIFIER_TEMPERATURE,
	BPSK_TONE_CURRENT,
	FM_POWER_AMPLIFIER_TEMPERATURE,
	BPSK_POWER_AMPLIFIER_CURRENT,
	FM_POWER_AMPLIFIER_CURRENT,
	/* EPS Board */
	EPS_PROCESSOR_TEMPERATURE,			
	STRUCTURE_TEMPERATURE,			
	EPS_DC_TO_DC_CONVERTER_TEMPERATURE,
	EPS_DC_TO_DC_CONVERTER_CURRENT,
	EPS_DC_TO_DC_CONVERTER_VOLTAGE,
	EPS_SMPS_6V_VOLTAGE,
	EPS_SMPS_9V_VOLTAGE,
	EPS_SMPS_3V_VOLTAGE,
	EPS_SMPS_6V_CURRENT,
	EPS_SMPS_3V_CURRENT,
	EPS_SMPS_9V_CURRENT,
}SENSOR_NAME;


#endif /* SENSORS_H_ */