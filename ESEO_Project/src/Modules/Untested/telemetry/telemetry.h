#ifndef __TELEMETRY_H__
#define __TELEMETRY_H__

#define	I2C_MODULE			(&AVR32_TWIM0) 
#define	I2C_BAUD			100000
#define I2C_EEPROM_ADDR		(0xa0>>1) // Correct CPB
#define I2C_TX_ADC_ADDR		(0x33) // DB: Same as RF on FC1, 2 on PAs
#define I2C_REC_ADC_ADDR	(0x34) // DB: Same as PA on FC1, LO
#define I2C_EPS_ADC_ADDR	(0x35) // Correct CPB, double verified with DB as SIB

status_code_t InitI2C(status_code_t status);

status_code_t TaskEEPROMtlm(status_code_t status);

void InitEPSADC(status_code_t* status);
void TaskEPSADC(status_code_t* status, Byte* ADCBuff );

void InitRecADC(status_code_t* status);
void TaskRecADC(status_code_t* status, Byte* ADCBuff );

#endif	// __TELEMETRY_H__