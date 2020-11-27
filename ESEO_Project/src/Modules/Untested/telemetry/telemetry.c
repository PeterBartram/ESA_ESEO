#include <asf.h>
#include "twim.h"
#include "conf_clock.h"
#include "baseTimer.h"
#include "telemetry.h"

status_code_t InitI2C(status_code_t status)
{
	const gpio_map_t TWIM_GPIO_MAP = {
		{AVR32_TWIMS0_TWCK_0_0_PIN, AVR32_TWIMS0_TWCK_0_0_FUNCTION},
		{AVR32_TWIMS0_TWD_0_0_PIN, AVR32_TWIMS0_TWD_0_0_FUNCTION}
	};
	
	const twi_options_t TWIM_OPTIONS = {
		.pba_hz = CLK_PBA_F,
		.speed = I2C_BAUD,
		.chip = I2C_EPS_ADC_ADDR,
		.smbus = false,
	};
	
	gpio_enable_module( TWIM_GPIO_MAP, sizeof(TWIM_GPIO_MAP)/sizeof(TWIM_GPIO_MAP[0]));
	status = twim_master_init( I2C_MODULE, &TWIM_OPTIONS );
	
	return status;
}

void InitEPSADC(status_code_t* status)
{
	twi_package_t writeADCinit;
	writeADCinit.chip = I2C_EPS_ADC_ADDR;
	writeADCinit.addr[0] = 0x17; // TODO: 3.3V > 2.128V
	writeADCinit.addr[1] = 0xd2;
	writeADCinit.addr_length = sizeof(writeADCinit.addr)/sizeof(uint8_t);
	writeADCinit.buffer = NULL;
	writeADCinit.length = 0;
	*status = twim_write_packet( I2C_MODULE, &writeADCinit );
	return;
}

void InitRecADC(status_code_t* status)
{
	twi_package_t writeADCinit;
	writeADCinit.chip = I2C_REC_ADC_ADDR;
	writeADCinit.addr[0] = 0x07; // TODO: 3.3V > 2.128V
	writeADCinit.addr[1] = 0xd2;
	writeADCinit.addr_length = sizeof(writeADCinit.addr)/sizeof(uint8_t);
	writeADCinit.buffer = NULL;
	writeADCinit.length = 0;
	*status = twim_write_packet( I2C_MODULE, &writeADCinit );
	return;
}

void TaskRecADC(status_code_t* status, Byte* ADCBuff )
{
	static int32_t timeCounter = 0;
	twi_package_t readPackADC;
	
	if( (GetBaseTimer()-timeCounter) > 5000 ) // Every 5 seconds
	{
		timeCounter = GetBaseTimer();
		
		readPackADC.chip = I2C_REC_ADC_ADDR;
		readPackADC.addr[0] = 0;
		readPackADC.addr[1] = 0;
		readPackADC.addr_length = 1;
		readPackADC.buffer = ADCBuff;
		readPackADC.length = sizeof(ADCBuff)/sizeof(uint8_t);
		*status = twim_read_packet( I2C_MODULE, &readPackADC );
		*ADCBuff = readPackADC.buffer;
		nop();
	}
	
	return;

}

void TaskEPSADC(status_code_t* status, Byte* ADCBuff )
{
	static int32_t timeCounter = 0;
	twi_package_t readPackADC;
	
	if( (GetBaseTimer()-timeCounter) > 5000 ) // Every 5 seconds
	{
		timeCounter = GetBaseTimer();
		
		readPackADC.chip = I2C_EPS_ADC_ADDR;
		readPackADC.addr[0] = 0;
		readPackADC.addr[1] = 0; 
		readPackADC.addr_length = 1;
		readPackADC.buffer = ADCBuff;
		readPackADC.length = sizeof(ADCBuff)/sizeof(uint8_t);
		*status = twim_read_packet( I2C_MODULE, &readPackADC );
		*ADCBuff = readPackADC.buffer;
		nop();
	}
	
	return;

}

status_code_t TaskEEPROMtlm(status_code_t status)
{
	static int32_t timeCounter = 0;
	int32_t delayCounter;
	twi_package_t writePack, readPack;
	uint8_t writeBuff[9] = {0x63,0x68,0x72,0x69,0x73,0x77,0x69,0x6e,0x73};
	uint8_t readBuff[9];
	
	if( (GetBaseTimer()-timeCounter) > 5000 ) // Every 5 seconds
	{
		timeCounter = GetBaseTimer();
		
		writePack.chip = I2C_EEPROM_ADDR;
		writePack.addr[0] = (0x0010 >> 8) & 0xFF;
		writePack.addr[1] = (0x0010 >> 0) & 0xFF;
		writePack.addr_length = sizeof(writePack.addr)/sizeof(uint8_t);
		writePack.buffer = writeBuff;
		writePack.length = sizeof(writeBuff)/sizeof(uint8_t);
		status = twim_write_packet( I2C_MODULE, &writePack );
		
		delayCounter = GetBaseTimer();
		while( (GetBaseTimer()-delayCounter) < 11 );
		
		readPack.chip = I2C_EEPROM_ADDR;
		readPack.addr[0] = (0x0010 >> 8) & 0xFF;
		readPack.addr[1] = (0x0010 >> 0) & 0xFF;
		readPack.addr_length = sizeof(readPack.addr)/sizeof(uint8_t);
		readPack.buffer = readBuff;
		readPack.length = sizeof(readBuff)/sizeof(uint8_t);
		status = twim_read_packet( I2C_MODULE, &readPack );
		
	}
	
	return status;
}

