/**
@file 	ADCDriver.c
@author Pete Bartram
@brief 
The intention of this module is to collect sensor data from an ADC based upon a SENSOR_NAME.
This is low level code that directly interacts with the ADCs, therefore it also configures
the I2C bus and handled error responses from the communications nodes.
*/
#include "Sensors.h"
#include "ADCDriver.h"
#include "asf.h"
#include "conf_clock.h"
#include "twim.h"

/* I2C Module Configuration Options */
#define	I2C_MODULE			(&AVR32_TWIM0)
#define	I2C_BAUD			100000

/* These are the I2C driver chip addresses for the various ADCs across all boards. */
#define BPSK_TRANSMITTER_ADC_ADDRESS 0x33
#define LBAND_RECEIVER_ADC_ADDRESS   0x34
#define EPS_BOARD_ADC_ADDRESS		 0x35
#define I2C_EEPROM_ADDR				 0x50

/* This is the maximum number of channels across all ADCs */
#define MAXIMUM_CHANNELS			12
#define NUMBER_OF_ADCS				3

typedef struct _CHANNEL_MAP
{
	uint8_t copyChannel;
	SENSOR_NAME sensorName;	
}CHANNEL_MAP;

/** Structure used for holding configuration data for a single ADC. */
typedef struct _ADC_CONFIG
{
	uint32_t I2CAddress;						/**< The I2C address of the ADC. */
	uint8_t	configurationByte;					/**< The channel configuration byte to send for setting up the ADC. */
	uint8_t setUpByte;							/**< The setup byte used for configuring an ADC. */
//	uint8_t	configured;							/**< If this is true then the ADC has been correctly configured and is ready for use. */
	uint8_t numberOfChannels;					/**< The number of configured channels on this ADC. */
	CHANNEL_MAP channelMap[MAXIMUM_CHANNELS];	/**< Contains the mapping from each channel read to the output buffer. */
}ADC_CONFIG;

/* 
	Contains all information on each ADC and also what channels to write to what location in
	the output buffer - this includes a flag for deciding whether to copy or not.
*/
ADC_CONFIG adcConfiguration[NUMBER_OF_ADCS] =
{
	{LBAND_RECEIVER_ADC_ADDRESS,	0x82, 0x07, 4, 
		{
			1, TRANSPONDER_RSSI, 
			1, COMMAND_DOPPLER_SHIFT, 
			1, COMMAND_RSSI,
			1, COMMAND_OSCILATTOR_TEMPERATURE
		},
	},
	{BPSK_TRANSMITTER_ADC_ADDRESS, 0x82, 0x0F, 8,
		{
			1, BPSK_REVERSE_POWER,
			1, BPSK_FORWARD_POWER,
			1, BPSK_POWER_AMPLIFIER_TEMPERATURE,
			0, (SENSOR_NAME)0,				
			1, BPSK_TONE_CURRENT,
			1, FM_POWER_AMPLIFIER_TEMPERATURE,
			1, BPSK_POWER_AMPLIFIER_CURRENT,
			1, FM_POWER_AMPLIFIER_CURRENT
		},
	},
	{EPS_BOARD_ADC_ADDRESS,	0x82, 0x15, 12,
		{
			1, EPS_PROCESSOR_TEMPERATURE,			
			1, STRUCTURE_TEMPERATURE,			
			1, EPS_DC_TO_DC_CONVERTER_TEMPERATURE,
			1, EPS_DC_TO_DC_CONVERTER_CURRENT,
			1, EPS_DC_TO_DC_CONVERTER_VOLTAGE,
			1, EPS_SMPS_6V_VOLTAGE,
			1, EPS_SMPS_9V_VOLTAGE,
			1, EPS_SMPS_3V_VOLTAGE,
			1, EPS_SMPS_6V_CURRENT,
			1, EPS_SMPS_3V_CURRENT,
			1, EPS_SMPS_9V_CURRENT,
		},
	}
};

uint16_t * p_returnData = 0;	/**< Diagnostics data buffer, all retrieved ADC data is to be returned to here. */
uint32_t returnDataSize = 0;	/**< The size of the buffer pointed to by p_diagnosticData */

/* Private functions. */
//static ADC_ERROR GetADCDiagnosticsData(&informationStructure);
static ADC_ERROR InitialiseI2C(void);
static ADC_ERROR ConfigureADC(uint32_t z_ADCNumber);
static ADC_ERROR ReadADC(uint32_t z_ADCNumber, uint16_t * z_outputBuffer);
static uint16_t ConvertADCTwoByteFormatToAValue(uint16_t z_value);
static ADC_ERROR TransferDataToOutput(uint32_t z_adcNumber, uint16_t * z_buffer, uint32_t z_bufferSize, uint16_t * z_outputBuffer, uint32_t z_outputBufferSize);

/**
*	Perform initalisation of the I2C bus, and also send a set up packet to each ADC.	
*	\param[out] z_interface: Pointer to the interface structure for future ADC usage.
*	\return ADC_ERROR code
*/
ADC_ERROR ADCInitialise(uint16_t * z_buffer, uint32_t z_size)
{
	ADC_ERROR retVal = NO_ERROR_ADC;
	
	if(z_buffer != 0 && z_size != 0)
	{
		if(InitialiseI2C() == NO_ERROR_ADC)
		{
			/* Store our output buffer location. */
			p_returnData = z_buffer;
			returnDataSize = z_size;
			retVal = NO_ERROR_ADC;
		}
		else
		{
			retVal = COULD_NOT_INITIALISE_ADC;
		}
	}
	else
	{
		retVal = NO_BUFFER_PROVIDED_ADC;
	}

	return retVal;
}

/**
*	Perform initalisation of the I2C bus and send a set-up packet to each specified ADC.
*	\return DIAGNOSTICS_ERROR code
*/
ADC_ERROR InitialiseI2C(void)
{
	ADC_ERROR retVal  = COULD_NOT_INITIALISE_ADC;
	
	/* Declared static to allow for testing. */
	static const gpio_map_t twim_gpio_map = {
		{AVR32_TWIMS0_TWCK_0_0_PIN, AVR32_TWIMS0_TWCK_0_0_FUNCTION},
		{AVR32_TWIMS0_TWD_0_0_PIN, AVR32_TWIMS0_TWD_0_0_FUNCTION}
	};

	/* Configure clock speeds and addresses for our I2C bus - Declared static to allow for testing. */
	static const twi_options_t twimOptions = {CLK_PBA_F, I2C_BAUD, EPS_BOARD_ADC_ADDRESS, 0};			//@todo not convinced that we should be giving the I2C address of another module here!
		
	/* Enable the twim module. */
	if(gpio_enable_module(twim_gpio_map, sizeof(twim_gpio_map)/sizeof(twim_gpio_map[0])) == GPIO_SUCCESS)
	{
		/* Initialise our I2C module */
		if(twim_master_init(I2C_MODULE, &twimOptions) == TWI_SUCCESS)
		{
			uint32_t numberOfADCs = sizeof(adcConfiguration)/sizeof(adcConfiguration[0]);
			uint32_t i = 0;

			/* Assume no error at this point. */
			retVal = NO_ERROR_ADC;
			
			/* Configure each of our ADCs in order. */
			for(i = 0; i < numberOfADCs; i++)
			{
				if(ConfigureADC(i) == NO_ERROR_ADC)
				{
					/* Record that this ADC has been configured successfully. */
				}	
				else
				{
					/* We failed to initialise at least one ADC - view the configured flags for diagnostics data. */
					retVal = COULD_NOT_INITIALISE_ADC;
				}			
			}
		}
	}
	return retVal;
}

ADC_ERROR ConfigureADC(uint32_t z_ADCNumber)
{
	ADC_ERROR retVal = INVALID_ADC_SPECIFIED;
	twi_package_t adcPacket;
	
	/* Ensure we haven't gotten a setup request for an ADC thats not in our table. */
	if(z_ADCNumber < sizeof(adcConfiguration)/sizeof(adcConfiguration[0]))
	{
		/* Configure our set/configuation packet to send. */
		adcPacket.chip = adcConfiguration[z_ADCNumber].I2CAddress;
		adcPacket.addr[0] = adcConfiguration[z_ADCNumber].configurationByte;
		adcPacket.addr[1] = adcConfiguration[z_ADCNumber].setUpByte;
		adcPacket.addr_length = 2;
		adcPacket.buffer = 0;
		adcPacket.length = 0;
	
		/* Send our setup/configuration packet. */
		if(twim_write_packet(I2C_MODULE, &adcPacket) == STATUS_OK)
		{
			retVal = NO_ERROR_ADC;
		}
	}
	return retVal;
}

ADC_ERROR TransferDataToOutput(uint32_t z_adcNumber, uint16_t * z_buffer, uint32_t z_bufferSize, uint16_t * z_outputBuffer, uint32_t z_outputBufferSize)
{
	ADC_ERROR retVal = INVALID_ADC_SPECIFIED;
	uint32_t i = 0;

	if(z_buffer != 0 && z_adcNumber < NUMBER_OF_ADCS)
	{
		retVal = NO_ERROR_ADC;

		for(i = 0; i < adcConfiguration[z_adcNumber].numberOfChannels; i++)
		{
			if(adcConfiguration[z_adcNumber].channelMap[i].copyChannel != 0)
			{
				if(z_outputBufferSize > (uint32_t)adcConfiguration[z_adcNumber].channelMap[i].sensorName)
				{
					if(z_bufferSize > i)
					{
						z_outputBuffer[(uint32_t)adcConfiguration[z_adcNumber].channelMap[i].sensorName] = ConvertADCTwoByteFormatToAValue(z_buffer[i]);
					}
				}
			}
		}
	}
	return retVal;
}

/**
*	Fetch telemetry data from a single ADC channel.	
*	\param[in] z_name: The sensor enumeration to fetch data from.
*	\param[out] z_buffer: The location to write telemetry data out to. 
*	\return ADC_ERROR code
*/
ADC_ERROR FetchDiagnosticsData(void)
{
	ADC_ERROR retVal = ADC_NOT_INITIALISED;
	uint16_t receiveBuffer[MAXIMUM_CHANNELS];

		/* Check that we have been initialised. */
	if(p_returnData != 0 && returnDataSize != 0)
	{
		uint32_t numberOfADCs = sizeof(adcConfiguration)/sizeof(adcConfiguration[0]);
		uint32_t i = 0;

		/* Assume no error at this point. */
		retVal = NO_ERROR_ADC;
			
		/* Read from each of our ADCs in order. */
		for(i = 0; i < numberOfADCs; i++)
		{
			if(ReadADC(i, receiveBuffer) == NO_ERROR_ADC)
			{
				TransferDataToOutput(i, receiveBuffer, sizeof(receiveBuffer)/sizeof(receiveBuffer[0]), p_returnData, returnDataSize);
				/* @todo Record that we read successfully */
			}
			else
			{
				retVal = COULD_NOT_READ_ADC;
			}	
		}
	}
	return retVal;
}

/**
*	Read data out of a single ADC channel.	
*	\param[in] z_name: The sensor enumeration to fetch data from.
*	\param[out] z_buffer: The location to write telemetry data out to. 
*	\return ADC_ERROR code
*/
ADC_ERROR ReadADC(uint32_t z_ADCNumber, uint16_t * z_outputBuffer)
{
	twi_package_t readADCPacket;
	ADC_ERROR retVal = INVALID_ADC_SPECIFIED;

	/* Ensure we haven't gotten a setup request for an ADC thats not in our table. */
	if(z_ADCNumber < sizeof(adcConfiguration)/sizeof(adcConfiguration[0]))
	{
		/* Configure our read data structure. */
		readADCPacket.addr_length = 0;
		readADCPacket.buffer = (void*)z_outputBuffer;
		readADCPacket.chip = adcConfiguration[z_ADCNumber].I2CAddress;
		readADCPacket.length = (adcConfiguration[z_ADCNumber].numberOfChannels * sizeof(uint16_t));	/* Times uint16_t to convert to 1 bits per reading. */

		/* Perform a read of the I2C bus. */
		if(twim_read_packet(I2C_MODULE, &readADCPacket) == STATUS_OK)
		{
			retVal = NO_ERROR_ADC;
		}
	}
	return retVal;
}

void ADCUninitialise(void)
{
	/* We can no longer assume this data store is valid - delete the reference. */
	p_returnData = 0;
	returnDataSize = 0;
}

/**
*	The ADC provides data in a two byte format with the 2MSBs of one byte being used along with the other byte 
*	to produce a 10 bit value that represents the analogue value.
*	This function will convert from this format into a usable value in a uint32_t.
*	\param[in] z_value: Value to convert.
*	\return Converted value.
@todo error checking has not been properly considered here!
*/
uint16_t ConvertADCTwoByteFormatToAValue(uint16_t z_value)
{
	uint16_t retVal = z_value & 0x03FF;
	return retVal;
}
