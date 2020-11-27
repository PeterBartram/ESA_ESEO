#include "gtest/gtest.h"

extern "C"
{
	#include "SatelliteOperations.h"
	#include "fff.h"
	#include "semphr.h"
	#include "Fake_semphr.h"
	#include "timers.h"
	#include "Fake_timers.h"
	#include "CANObjectDictionary.h"
	#include "Fake_CANObjectDictionary.h"
	#include "Fake_CANODInterface.h"
	#include "CANODInterface.h"
	#include "flashc.h"
	#include "Fake_flashc.h"
	#include "Fake_queue.h"
	#include "Fake_gpio.h"
	#include "Fake_Downlink.h"
	#include "Fake_DACout.h"
	#include "WOD.h"
}

DEFINE_FFF_GLOBALS;

class SatelliteOperations : public testing::Test
{
public:


	void SetUp()
	{
		RESET_FAKE(GetCANObjectDataRaw)
		InitialiseWOD();
	}

	virtual void TearDown() 
	{

	}
};

uint32_t sampleNumber = 0;
static OBJECT_DICTIONARY_ERROR	GetCANObjectDataRaw_FakeReturn(uint32_t z_index, uint32_t * z_data);
static OBJECT_DICTIONARY_ERROR	GetCANObjectDataRaw_FakeReturn(uint32_t z_index, uint32_t * z_data)
{
	if(z_data)
	{
		*z_data = sampleNumber;
	}
	return NO_ERROR_OBJECT_DICTIONARY;
}

TEST_F(SatelliteOperations, TestWeAddAndRemoveCorrectDataWithoutLoop)
{
	uint8_t buffer[200];

	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRaw_FakeReturn;

	for(sampleNumber = 0; sampleNumber < 100; sampleNumber++)
	{
		UpdateWODData();
	}
	
	GetWODData(1, buffer);
	EXPECT_EQ(0, buffer[0]);
	EXPECT_EQ(4, buffer[160]);
	GetWODData(20, buffer);
	EXPECT_EQ(95, buffer[0]);
	EXPECT_EQ(99, buffer[160]);
}

TEST_F(SatelliteOperations, TestWeAddAndRemoveCorrectDataWithLoop)
{
	uint8_t buffer[200];

	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRaw_FakeReturn;

	for(sampleNumber = 0; sampleNumber < 66; sampleNumber++)
	{
		UpdateWODData();
	}

	for(sampleNumber = 0; sampleNumber < 100; sampleNumber++)
	{
		UpdateWODData();
	}

	GetWODData(1, buffer);
	EXPECT_EQ(0, buffer[0]);
	EXPECT_EQ(4, buffer[160]);
	GetWODData(20, buffer);
	EXPECT_EQ(95, buffer[0]);
	EXPECT_EQ(99, buffer[160]);
}












//#include "gtest/gtest.h"
//
//extern "C"
//{
//	#include "SatelliteOperations.h"
//	#include "fff.h"
//	#include "semphr.h"
//	#include "Fake_semphr.h"
//	#include "timers.h"
//	#include "Fake_timers.h"
//	#include "CANObjectDictionary.h"
//	#include "Fake_CANObjectDictionary.h"
//	#include "Fake_CANODInterface.h"
//	#include "CANODInterface.h"
//	#include "flashc.h"
//	#include "Fake_flashc.h"
//	#include "Fake_queue.h"
//	#include "Fake_gpio.h"
//	#include "Fake_Downlink.h"
//	#include "Fake_DACout.h"
//}
//#define FLASH_PAGE_SIZE 128
//#define BPSK_ON_OFF_SWITCH				AVR32_PIN_PC17
//#define TRANSPONDER_ON_OFF_SWITCH		AVR32_PIN_PC18
//#define TRANSPONDER_POWER_SWITCH		AVR32_PIN_PB02
//#define ANTENNA_SWITCH					AVR32_PIN_PC19
//#define DECODER_TONE_INPUT				AVR32_PIN_PB28
//#define NUMBER_OF_FIELDS_USED_BY_PROCESSOR 5	// Fields in the user page used by the processor for fuses.
//
//typedef struct _DEFAULT_VALUES_STRUCTURE
//{
//	uint32_t fieldUsedInternallyByTheProcessor[NUMBER_OF_FIELDS_USED_BY_PROCESSOR];
//	uint32_t rfMode;
//	uint32_t dataMode;
//	uint32_t eclipseThresholdValue;
//	uint32_t autonomousModeDefault;
//	uint32_t sequenceNumber;
//	uint32_t flashUpdateRate;
//	uint32_t dataValid;
//	uint32_t pageFiller[FLASH_PAGE_SIZE - 12];	// Make sure the next variable is not in the same flash page.
//	uint32_t dataInvalid;
//	uint32_t pageFiller1[FLASH_PAGE_SIZE - 1];	// Make sure this array takes up the whole flash page.
//}DEFAULT_VALUES_STRUCTURE;
//
//DEFINE_FFF_GLOBALS;
//#define NUMBER_OF_WODS  5
//#define SIZE_OF_WOD		39
//
//extern void (*p_UpdateWODData)(void);
//extern void (*p_WodTakeSample)(xTimerHandle z_timer);
//extern void (*p_updateDefaultValues)(xTimerHandle z_timer);
//extern void (*p_sampleSequenceNumber)(xTimerHandle z_timer);
//extern uint8_t * p_eclipseTimeExpiredFlag;
//
//extern DEFAULT_VALUES_STRUCTURE * p_payloadDataStore;
//
//uint8_t buffer[195];
//uint32_t callCount;
//uint32_t wodSampleCount;
//uint32_t count;
//uint8_t outputBuffer[6];
//
//uint8_t incrementingWOD[39] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,101,1,46 ,16 ,100,104,108,112,116,120,124,228,248,8,1};
//
//static OBJECT_DICTIONARY_ERROR GetCANObjectDataRawCount( uint32_t z_index, uint32_t * z_data);
//static OBJECT_DICTIONARY_ERROR GetCANObjectDataRawReturn5(uint32_t z_index, uint32_t * z_data);
//static uint32_t xQueueSendFAKE(QueueHandle_t z_queue, const void * z_data, uint32_t z_ticks);
//static OBJECT_DICTIONARY_ERROR GetCANObjectDataRawReturnStartUp(uint32_t z_index, uint32_t * z_data);
//uint32_t returnQueueStartUp[100];
//uint32_t returnStartUpIndex;
//
//class SatelliteOperations : public testing::Test
//{
//public:
//
//
//	void SetUp()
//	{
//		RESET_FAKE(TimerCreate);
//		RESET_FAKE(TimerStart);
//		RESET_FAKE(xSemaphoreCreateRecursiveMutexStatic);
//		RESET_FAKE(xSemaphoreTakeRecursive);
//		RESET_FAKE(xSemaphoreGiveRecursive);		
//		RESET_FAKE(GetCANObjectDataRaw);
//		RESET_FAKE(SetCANObjectDataRaw);
//		RESET_FAKE(xSemaphoreCreateRecursiveMutexStatic);
//		RESET_FAKE(xSemaphoreGiveRecursive);
//		RESET_FAKE(xSemaphoreTakeRecursive);
//		RESET_FAKE(ClearPage);
//		RESET_FAKE(ClearPageBuffer);
//		RESET_FAKE(WriteToFlash);
//		RESET_FAKE(xQueueSend);
//		RESET_FAKE(GetCommandQueueHandle);
//		RESET_FAKE(gpio_enable_module);
//		RESET_FAKE(gpio_configure_pin);
//		RESET_FAKE(gpio_set_pin_low);
//		RESET_FAKE(gpio_set_pin_high);
//		RESET_FAKE(gpio_get_pin_value);
//		RESET_FAKE(ChangeDownlinkMode);
//		RESET_FAKE(ChangeMode);
//
//		memset(returnQueueStartUp, 0, sizeof(returnQueueStartUp));
//		GetCommandQueueHandle_fake.return_val = 1;
//		xSemaphoreTakeRecursive_fake.return_val = 1;
//		memset(p_payloadDataStore, 0, sizeof(DEFAULT_VALUES_STRUCTURE)*2);
//		xSemaphoreCreateRecursiveMutexStatic_fake.return_val = 1;
//		InitialiseSatelliteOperations(1);
//		memset(buffer, 0, sizeof(buffer));
//		memset(outputBuffer, 0, sizeof(outputBuffer));
//		returnStartUpIndex = 0;
//		callCount = 0;
//		count = 0;
//		wodSampleCount = 0;
//		gpio_get_pin_value_fake.return_val = 1;
//	}
//
//	virtual void TearDown() 
//	{
//
//	}
//};
//
//
//static OBJECT_DICTIONARY_ERROR GetCANObjectDataRawReturnStartUp(uint32_t z_index, uint32_t * z_data)
//{
//	if(z_data != 0)
//	{
//		*z_data = returnQueueStartUp[returnStartUpIndex];
//		returnStartUpIndex++;
//	}
//	return NO_ERROR_OBJECT_DICTIONARY;
//}
//
//static OBJECT_DICTIONARY_ERROR GetCANObjectDataRawReturn5(uint32_t z_index, uint32_t * z_data)
//{
//	*z_data = 5;
//	return NO_ERROR_OBJECT_DICTIONARY;
//}
//
//static uint32_t xQueueSendFAKE(QueueHandle_t z_queue, const void * z_data, uint32_t z_ticks)
//{
//	outputBuffer[0] = ((uint8_t*)z_data)[0];
//	outputBuffer[1]=  ((uint8_t*)z_data)[1];
//	outputBuffer[2] = ((uint8_t*)z_data)[2];
//	outputBuffer[3] = ((uint8_t*)z_data)[3];
//	outputBuffer[4] = ((uint8_t*)z_data)[4];
//	outputBuffer[5] = ((uint8_t*)z_data)[5];
//	outputBuffer[6] = ((uint8_t*)z_data)[6];
//	return 1;
//}
//
//OBJECT_DICTIONARY_ERROR GetCANObjectDataRawFAKE(uint32_t z_index, uint32_t * z_data);
//OBJECT_DICTIONARY_ERROR GetCANObjectDataRawFAKE2(uint32_t z_index, uint32_t * z_data);
//
//OBJECT_DICTIONARY_ERROR GetCANObjectDataRawFAKE(uint32_t z_index, uint32_t * z_data)
//{
//	*z_data = callCount;
//	callCount++;
//	return NO_ERROR_OBJECT_DICTIONARY;
//}
//
//OBJECT_DICTIONARY_ERROR GetCANObjectDataRawFAKE2(uint32_t z_index, uint32_t * z_data)
//{
//	static uint32_t lastIndex = 0;
//	/* Ignore requests from the operating modes section */
//	if(z_index != AMS_OBC_RFMODE && z_index != AMS_OBC_DATAMODE && z_index != AMS_OBC_CTCSS_MODE
//	&& z_index != AMS_OBC_ECLIPSE_MODE && z_index != AMS_OBC_ECLIPSE_PANELS&& z_index != PPM_VOLTAGE_SP1
//	&& z_index != PPM_VOLTAGE_SP2 && z_index != PPM_VOLTAGE_SP3 && z_index != AMS_OBC_ECLIPSE_THRESH
//	&& z_index !=AMS_OBC_AUTO_SAFE_MODE && z_index != AMS_OBC_SAFE_MODE_ACTIVE)
//	{
//		if((z_index != AMS_EPS_DCDC_T) || (z_index == AMS_EPS_DCDC_T && lastIndex != AMS_OBC_SAFE_MODE_ACTIVE))
//		{
//			*z_data = wodSampleCount;
//			callCount++;
//			if(callCount >= 41)		// Number of calls to can od get per wod sample
//			{
//				callCount = 0;
//				wodSampleCount++;
//			}
//		}
//	}
//	lastIndex = z_index;
//	return NO_ERROR_OBJECT_DICTIONARY;
//}
//
///***************************************************/
///* General initialisation testing.				   */
///***************************************************/
//TEST_F(SatelliteOperations, TestThatInitialiseSatelliteOperationsCallsTimerCreate)
//{
//	EXPECT_EQ(7, TimerCreate_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialiseSatelliteOperationsCallsTimerCreateWithCorrectParametersForWODTimer)
//{
//	EXPECT_NE(0, (int)TimerCreate_fake.arg0_history[1]);
//	EXPECT_EQ(60000,(int)TimerCreate_fake.arg1_history[1]);
//	EXPECT_EQ(1, (int)TimerCreate_fake.arg2_history[1]);
//	EXPECT_NE(0, (int)TimerCreate_fake.arg3_history[1]);
//	EXPECT_NE(0, (int)TimerCreate_fake.arg4_history[1]);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialiseSatelliteOperationsCallsTimerCreateWithCorrectParametersForDefaultValueUpdateTimer)
//{
//	EXPECT_NE(0, (int)TimerCreate_fake.arg0_history[0]);
//	EXPECT_EQ(1800000,(int)TimerCreate_fake.arg1_history[0]);
//	EXPECT_EQ(1, (int)TimerCreate_fake.arg2_history[0]);
//	EXPECT_NE(0, (int)TimerCreate_fake.arg3_history[0]);
//	EXPECT_NE(0, (int)TimerCreate_fake.arg4_history[0]);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationCreatesASemaphore)
//{
//	EXPECT_EQ(1, xSemaphoreCreateRecursiveMutexStatic_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationCallsConfigureGpioCorrectNumberOfTimes)
//{
//	EXPECT_EQ(5, gpio_configure_pin_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationCallsConfigureGpioForCorrectPins)
//{
//	EXPECT_EQ(81, gpio_configure_pin_fake.arg0_history[0]);
//	EXPECT_EQ(82, gpio_configure_pin_fake.arg0_history[1]);
//	EXPECT_EQ(34, gpio_configure_pin_fake.arg0_history[2]);
//	EXPECT_EQ(83, gpio_configure_pin_fake.arg0_history[3]);
//	EXPECT_EQ(60, gpio_configure_pin_fake.arg0_history[4]);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationCallsGetCANObjectDictionaryCorrectNumberOfTimes)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	RESET_FAKE(GetCANObjectDataRaw);
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(4, GetCANObjectDataRaw_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationCallsGetCANObjectDictionaryWithCorrectValues)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	RESET_FAKE(GetCANObjectDataRaw);
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(AMS_OBC_RFMODE, GetCANObjectDataRaw_fake.arg0_history[0]);
//	EXPECT_NE((uint32_t*)0, GetCANObjectDataRaw_fake.arg1_history[0]);
//
//	EXPECT_EQ(AMS_OBC_DATAMODE, GetCANObjectDataRaw_fake.arg0_history[1]);
//	EXPECT_NE((uint32_t*)0, GetCANObjectDataRaw_fake.arg1_history[1]);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationSetsGPIOCorrectlyForReceviveOnlyMode)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 0;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	ChangeDownlinkMode_fake.arg0_history[0] = MODE_1K2;
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[0]);
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[1]);
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[2]);
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[3]);
//	EXPECT_EQ(0, gpio_configure_pin_fake.arg1_history[4]);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationSetsGPIOCorrectlyForLowPowerTelemetry)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(3, gpio_configure_pin_fake.arg1_history[0]);		// BPSK
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[1]);		// Transponder
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[2]);		// Trandsponder Power
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[3]);		// Antenna 
//	EXPECT_EQ(0, gpio_configure_pin_fake.arg1_history[4]);		// Decoder
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationSetsGPIOCorrectlyForHighPowerTelemetry)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 2;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(3, gpio_configure_pin_fake.arg1_history[0]);		// BPSK
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[1]);		// Transponder
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[2]);		// Trandsponder Power
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[3]);		// Antenna 
//	EXPECT_EQ(0, gpio_configure_pin_fake.arg1_history[4]);		// Decoder
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationSetsGPIOCorrectlyForLowPowerTransponder)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 3;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[0]);		// BPSK
//	EXPECT_EQ(3, gpio_configure_pin_fake.arg1_history[1]);		// Transponder
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[2]);		// Trandsponder Power
//	EXPECT_EQ(3, gpio_configure_pin_fake.arg1_history[3]);		// Antenna 
//	EXPECT_EQ(0, gpio_configure_pin_fake.arg1_history[4]);		// Decoder
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationSetsGPIOCorrectlyForHighPowerTransponder)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 4;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[0]);		// BPSK
//	EXPECT_EQ(3, gpio_configure_pin_fake.arg1_history[1]);		// Transponder
//	EXPECT_EQ(3, gpio_configure_pin_fake.arg1_history[2]);		// Trandsponder Power
//	EXPECT_EQ(3, gpio_configure_pin_fake.arg1_history[3]);		// Antenna 
//	EXPECT_EQ(0, gpio_configure_pin_fake.arg1_history[4]);		// Decoder
//	EXPECT_EQ(MODE_RX_ONLY, ChangeDownlinkMode_fake.arg0_history[0]);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationSetsGPIOCorrectlyForAutonomousModeA)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[0]);		// BPSK
//	EXPECT_EQ(3, gpio_configure_pin_fake.arg1_history[1]);		// Transponder
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[2]);		// Trandsponder Power
//	EXPECT_EQ(3, gpio_configure_pin_fake.arg1_history[3]);		// Antenna 
//	EXPECT_EQ(0, gpio_configure_pin_fake.arg1_history[4]);		// Decoder
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationSetsGPIOCorrectlyForAutonomousModeB)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(3, gpio_configure_pin_fake.arg1_history[0]);		// BPSK
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[1]);		// Transponder
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[2]);		// Trandsponder Power
//	EXPECT_EQ(1, gpio_configure_pin_fake.arg1_history[3]);		// Antenna 
//	EXPECT_EQ(0, gpio_configure_pin_fake.arg1_history[4]);		// Decoder
//}
//
///*
//ChangeDownlinkMode - changes to data sent to the DAC from the downlink thread.
//ChangeMode - changes the actual data rate + power of the dac itself.
//*/
//
///*
//	MODE_1K2_LOW_POWER,
//	MODE_1K2_HIGH_POWER,
//	MODE_4K8_LOW_POWER,
//	MODE_4K8_HIGH_POWER,
//*/
//
//TEST_F(SatelliteOperations, TestThatInitialisationSwitchesToCorrectDataRate1K2LowPower)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	ChangeMode_fake.arg0_history[0] = MODE_4K8_HIGH_POWER;
//	ChangeDownlinkMode_fake.arg0_history[0] = MODE_4K8;
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(MODE_1K2_LOW_POWER, ChangeMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeMode_fake.call_count);
//	EXPECT_EQ(MODE_1K2, ChangeDownlinkMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeDownlinkMode_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationSwitchesToCorrectDataRate1k2HighPower)
//{
//	p_payloadDataStore[0].dataValid = 2;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 2;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	ChangeMode_fake.arg0_history[0] = MODE_4K8_HIGH_POWER;
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(MODE_1K2_HIGH_POWER, ChangeMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeMode_fake.call_count);
//	EXPECT_EQ(MODE_1K2, ChangeDownlinkMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeDownlinkMode_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationSwitchesToCorrectDataRate4K8LowPower)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 1;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	ChangeMode_fake.arg0_history[0] = MODE_1K2_LOW_POWER;
//	ChangeDownlinkMode_fake.arg0_history[0] = MODE_4K8;
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(MODE_4K8_LOW_POWER, ChangeMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeMode_fake.call_count);
//	EXPECT_EQ(MODE_4K8, ChangeDownlinkMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeDownlinkMode_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationSwitchesToCorrectDataRate4K8HighPower)
//{
//	p_payloadDataStore[0].dataValid = 2;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 2;	// RF Mode
//	returnQueueStartUp[1] = 1;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	ChangeMode_fake.arg0_history[0] = MODE_1K2_LOW_POWER;
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(MODE_4K8_HIGH_POWER, ChangeMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeMode_fake.call_count);
//	EXPECT_EQ(MODE_4K8, ChangeDownlinkMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeDownlinkMode_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationSwitchesToSendNoDataWhenInTransponderModeLowPower)
//{
//	p_payloadDataStore[0].dataValid = 2;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 3;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	ChangeMode_fake.arg0_history[0] = MODE_1K2_LOW_POWER;
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(MODE_RX_ONLY, ChangeDownlinkMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeDownlinkMode_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationSwitchesToSendNoDataWhenInTransponderModeHighPower)
//{
//	p_payloadDataStore[0].dataValid = 2;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 4;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	ChangeMode_fake.arg0_history[0] = MODE_1K2_LOW_POWER;
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(MODE_RX_ONLY, ChangeDownlinkMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeDownlinkMode_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationPutsAutonomousModeAIntoTransponderModeToBegin)
//{
//	p_payloadDataStore[0].dataValid = 2;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	ChangeMode_fake.arg0_history[0] = MODE_1K2_LOW_POWER;
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(MODE_RX_ONLY, ChangeDownlinkMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeDownlinkMode_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationPutsAutonomousModeBIntoHighPower1K2TelemetryModeToBegin)
//{
//	p_payloadDataStore[0].dataValid = 2;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	ChangeMode_fake.arg0_history[0] = MODE_1K2_LOW_POWER;
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(MODE_1K2, ChangeDownlinkMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeDownlinkMode_fake.call_count);
//	EXPECT_EQ(MODE_1K2_HIGH_POWER, ChangeMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeMode_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationConfigureGPIOCorrectlyLowPowerTelemetry1k2)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(1, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(3, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_low_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_low_fake.arg0_history[2]); 
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationConfigureGPIOCorrectlyHighPowerTelemetry1k2)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 2;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(1, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(3, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_low_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_low_fake.arg0_history[2]); 
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationConfigureGPIOCorrectlyLowPowerTelemetry4k8)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 1;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(1, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(3, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_low_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_low_fake.arg0_history[2]); 
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationConfigureGPIOCorrectlyHighPowerTelemetry4k8)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 2;	// RF Mode
//	returnQueueStartUp[1] = 1;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(1, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(3, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_low_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_low_fake.arg0_history[2]); 
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationConfigureGPIOCorrectlyLowPowerTransponder)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 3;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(2, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(2, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_low_fake.arg0_history[1]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_high_fake.arg0_history[1]); 
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationConfigureGPIOCorrectlyHighPowerTransponder)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 4;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//
//	InitialiseSatelliteOperations(1);
//	EXPECT_EQ(3, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(1, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_high_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_high_fake.arg0_history[2]); 
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationConfiguring1k2ModeAndThenChangingTo4k8ChangesMode)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 1;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 0;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 0;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	SatelliteOperationsTask();
//	EXPECT_EQ(MODE_4K8, ChangeDownlinkMode_fake.arg0_history[1]);
//	EXPECT_EQ(2, ChangeDownlinkMode_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationConfiguringLowPowerModeAndThenChangingToHighPowerChangesMode)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(gpio_configure_pin);
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 2;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 0;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 0;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	SatelliteOperationsTask();
//	EXPECT_EQ(MODE_1K2_HIGH_POWER, ChangeMode_fake.arg0_history[1]);
//	EXPECT_EQ(2, ChangeMode_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationCallsTimerCreate)
//{
//	EXPECT_EQ(7, TimerCreate_fake.call_count);
//	EXPECT_NE(0, (int)TimerCreate_fake.arg0_history[3]);
//	EXPECT_EQ(10000,(int)TimerCreate_fake.arg1_history[3]);
//	EXPECT_EQ(0, (int)TimerCreate_fake.arg2_history[3]);
//	EXPECT_EQ(0, (int)TimerCreate_fake.arg3_history[3]);
//	EXPECT_NE(0, (int)TimerCreate_fake.arg4_history[3]);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationCallsTimerCreateForCTCSSLost)
//{
//	EXPECT_NE(0, (int)TimerCreate_fake.arg0_history[4]);
//	EXPECT_EQ(20000,(int)TimerCreate_fake.arg1_history[4]);
//	EXPECT_EQ(0, (int)TimerCreate_fake.arg2_history[4]);
//	EXPECT_EQ(0, (int)TimerCreate_fake.arg3_history[4]);
//	EXPECT_NE(0, (int)TimerCreate_fake.arg4_history[4]);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationCallsTimerCreateForCTCSSOperating)
//{
//	EXPECT_NE(0, (int)TimerCreate_fake.arg0_history[5]);
//	EXPECT_EQ(300000,(int)TimerCreate_fake.arg1_history[5]);
//	EXPECT_EQ(0, (int)TimerCreate_fake.arg2_history[5]);
//	EXPECT_EQ(0, (int)TimerCreate_fake.arg3_history[5]);
//	EXPECT_NE(0, (int)TimerCreate_fake.arg4_history[5]);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationAutoModeAStartsATimerWhenWeGoIntoSunlight)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 1;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(TimerReset);
//	xTimerIsTimerActive_fake.return_val = 0;
//	SatelliteOperationsTask();
//	EXPECT_EQ(1, TimerReset_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatTimerIsResetIfWeChangeState)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 1;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(TimerReset);
//	xTimerIsTimerActive_fake.return_val = 0;
//	SatelliteOperationsTask();
//	returnStartUpIndex = 0;		// Reset return index.
//	xTimerIsTimerActive_fake.return_val = 1;
//	SatelliteOperationsTask();
//	EXPECT_EQ(1, TimerReset_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatIfWeDontChangeStateThenTimerIsNotReset)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 1;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(TimerReset);
//	SatelliteOperationsTask();
//	returnStartUpIndex = 0;		// Reset return index.
//	SatelliteOperationsTask();
//	SatelliteOperationsTask();
//	EXPECT_EQ(1, TimerReset_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatIfTimerIsResetIfWeChangeStateFromEclipseToSunlight)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 1;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(TimerReset);
//	xTimerIsTimerActive_fake.return_val = 0;
//	SatelliteOperationsTask();
//	returnStartUpIndex = 0;		// Reset return index.
//	xTimerIsTimerActive_fake.return_val = 1;
//	SatelliteOperationsTask();
//	EXPECT_EQ(1, TimerReset_fake.call_count);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 2;	// Panel 0
//	returnQueueStartUp[6] = 2;  // Panel 1
//	returnQueueStartUp[7] = 2;  // Panel 2
//	returnQueueStartUp[8] = 1;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	SatelliteOperationsTask();
//	EXPECT_EQ(2, TimerReset_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatWeChangeModesIfTimerExpiresWhenInEclipse)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 1;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(TimerReset);
//	SatelliteOperationsTask();
//	returnStartUpIndex = 0;		// Reset return index.
//	*p_eclipseTimeExpiredFlag = 1;
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	SatelliteOperationsTask();
//	EXPECT_EQ(3, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(1, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_high_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_high_fake.arg0_history[2]); 
//}
//
//TEST_F(SatelliteOperations, TestThatWeChangeModesIfTimerExpiresWhenInSunlight)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 0;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 2;	// Panel 0
//	returnQueueStartUp[6] = 2;  // Panel 1
//	returnQueueStartUp[7] = 2;  // Panel 2
//	returnQueueStartUp[8] = 1;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(TimerReset);
//	SatelliteOperationsTask();
//	returnStartUpIndex = 0;		// Reset return index.
//	*p_eclipseTimeExpiredFlag = 1;
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	SatelliteOperationsTask();
//	EXPECT_EQ(1, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(3, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_low_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_low_fake.arg0_history[2]); 
//}
//
//TEST_F(SatelliteOperations, TestThatWeChangeModeIntoEclipseTurnsOnTransponderIfCTCSSToneIsEnabledAndPresent)
//{
//	uint8_t retVal[2] = {0,0};
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 1;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 1;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 1;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(TimerReset);
//	SatelliteOperationsTask();
//	returnStartUpIndex = 0;		// Reset return index.
//	*p_eclipseTimeExpiredFlag = 1;
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	gpio_get_pin_value_fake.return_val_seq = retVal;
//	gpio_get_pin_value_fake.return_val_seq_len = 2;
//	SatelliteOperationsTask();
//	EXPECT_EQ(3, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(1, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_high_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_high_fake.arg0_history[2]); 
//}
//
//TEST_F(SatelliteOperations, TestThatWeChangeModeIntoEclipseDoesntTurnOnTransponderIfCTCSSToneIsEnabledAndNotPresent)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 1;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 1;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 1;  // Panel threshold
//	RESET_FAKE(TimerReset);
//	SatelliteOperationsTask();
//	returnStartUpIndex = 0;		// Reset return index.
//	*p_eclipseTimeExpiredFlag = 1;
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	gpio_get_pin_value_fake.return_val = 1;
//	SatelliteOperationsTask();
//	EXPECT_EQ(2, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(2, gpio_set_pin_low_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatWeSwitchBackAndFourthBetweenTelemetryAndTransponderIfCTCSSToneIsEnabledAndPresentAndWeChangeSunlight)
//{
//	uint8_t retVal[2] = {0,0};
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 1;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 1;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 1;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(TimerReset);
//	SatelliteOperationsTask();
//	returnStartUpIndex = 0;		// Reset return index.
//	*p_eclipseTimeExpiredFlag = 1;
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	gpio_get_pin_value_fake.return_val_seq = retVal;
//	gpio_get_pin_value_fake.return_val_seq_len = 2;
//	SatelliteOperationsTask();
//	EXPECT_EQ(3, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(1, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_high_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_high_fake.arg0_history[2]); 
//
//	/* Move out of eclipse */
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 1;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 0;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(TimerReset);
//	SatelliteOperationsTask();
//	returnStartUpIndex = 0;		// Reset return index.
//	*p_eclipseTimeExpiredFlag = 1;
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	gpio_get_pin_value_fake.return_val_seq = retVal;
//	gpio_get_pin_value_fake.return_val_seq_len = 2;
//	SatelliteOperationsTask();
//	EXPECT_EQ(1, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(3, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_low_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_low_fake.arg0_history[2]); 
//}
//
//TEST_F(SatelliteOperations, TestThatWeSwitchBackAndFourthBetweenTelemetryAndNothingIfCTCSSToneIsEnabledAndNotPresentAndWeChangeSunlight)
//{
//	uint8_t retVal[7] = {1,1,1,1,1,1,1};
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 1;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 1;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 1;  // Panel threshold
//	RESET_FAKE(TimerReset);
//	SatelliteOperationsTask();
//	returnStartUpIndex = 0;		// Reset return index.
//	*p_eclipseTimeExpiredFlag = 1;
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	gpio_get_pin_value_fake.return_val_seq = retVal;
//	gpio_get_pin_value_fake.return_val_seq_len = 7;
//	SatelliteOperationsTask();
//	EXPECT_EQ(2, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(2, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[1]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_high_fake.arg0_history[1]); 
//
//	/* Move out of eclipse */
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[2] = 1;	// CTCSS mode
//	returnQueueStartUp[3] = 0;	// Autonomous mode A/B select
//	returnQueueStartUp[4] = 7;	// Eclipse panels
//	returnQueueStartUp[5] = 0;	// Panel 0
//	returnQueueStartUp[6] = 0;  // Panel 1
//	returnQueueStartUp[7] = 0;  // Panel 2
//	returnQueueStartUp[8] = 0;  // Panel threshold
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(TimerReset);
//	SatelliteOperationsTask();
//	returnStartUpIndex = 0;		// Reset return index.
//	*p_eclipseTimeExpiredFlag = 1;
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	gpio_get_pin_value_fake.return_val_seq = retVal;
//	gpio_get_pin_value_fake.return_val_seq_len = 7;
//	SatelliteOperationsTask();
//	EXPECT_EQ(1, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(3, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_low_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_low_fake.arg0_history[2]); 
//}
//
///***************************************************/
///* Autonomous mode B testing					   */
///***************************************************/
//TEST_F(SatelliteOperations, AutoBMode_CTCSSToneCausesSwitchToTransponderMode)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	xTimerIsTimerActive_fake.return_val = 0;
//	gpio_get_pin_value_fake.return_val = 0;
//	RESET_FAKE(TimerReset);
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	SatelliteOperationsTask();
//	EXPECT_EQ(1, TimerReset_fake.call_count);
//	EXPECT_EQ(3, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(1, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_high_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_high_fake.arg0_history[2]); 
//}
//
//TEST_F(SatelliteOperations, AutoBMode_NoCTCSSToneCausesTelemetryModeToRemain)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	xTimerIsTimerActive_fake.return_val = 0;
//	gpio_get_pin_value_fake.return_val = 1;
//	RESET_FAKE(TimerReset);
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	SatelliteOperationsTask();
//	EXPECT_EQ(0, TimerReset_fake.call_count);
//	EXPECT_EQ(0, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(0, gpio_set_pin_low_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, AutoBMode_CTCSSToneDuringCooldownPeriodCausesTelemetryModeToRemain)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	xTimerIsTimerActive_fake.return_val = 1;
//	gpio_get_pin_value_fake.return_val = 1;
//	RESET_FAKE(TimerReset);
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	SatelliteOperationsTask();
//	EXPECT_EQ(0, TimerReset_fake.call_count);
//	EXPECT_EQ(0, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(0, gpio_set_pin_low_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, AutoBMode_CTCSSToneInTransponderModeForLessThan5MinsDoesntCauseAChange)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	xTimerIsTimerActive_fake.return_val = 0;
//	gpio_get_pin_value_fake.return_val = 1;
//	SatelliteOperationsTask();
//	RESET_FAKE(TimerReset);
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	xTimerIsTimerActive_fake.return_val = 1;
//	returnStartUpIndex = 0;		// Reset return index.
//	SatelliteOperationsTask();
//	EXPECT_EQ(0, TimerReset_fake.call_count);
//	EXPECT_EQ(0, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(0, gpio_set_pin_low_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, AutoBMode_5MinutesInTransponderModeAndWeSwitchBackToTelemetryMode)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	xTimerIsTimerActive_fake.return_val = 0;
//	gpio_get_pin_value_fake.return_val = 0;
//	SatelliteOperationsTask();
//	RESET_FAKE(TimerReset);
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	xTimerIsTimerActive_fake.return_val = 0;
//	returnStartUpIndex = 0;		// Reset return index.
//	SatelliteOperationsTask();
//	EXPECT_EQ(1, TimerReset_fake.call_count);
//	EXPECT_EQ(1, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(3, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_low_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_low_fake.arg0_history[2]); 
//}
//
//TEST_F(SatelliteOperations, AutoBMode_CTCSSToneLostInInTransponderModeForLessThan20SecondsDoesntCauseAChange)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	xTimerIsTimerActive_fake.return_val = 0;
//	gpio_get_pin_value_fake.return_val = 1;
//	SatelliteOperationsTask();
//	RESET_FAKE(TimerReset);
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	xTimerIsTimerActive_fake.return_val = 1;
//	returnStartUpIndex = 0;		// Reset return index.
//	gpio_get_pin_value_fake.return_val = 0;
//	SatelliteOperationsTask();
//	EXPECT_EQ(0, TimerReset_fake.call_count);
//	EXPECT_EQ(0, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(0, gpio_set_pin_low_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, AutoBMode_CTCSSToneLostInInTransponderModeFor20SecondsCausesChangeBackIntoTelemetryMode)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	RESET_FAKE(gpio_configure_pin);
//	TimerCreate_fake.return_val = (xTimerHandle)1;
//	InitialiseSatelliteOperations(1);
//	returnStartUpIndex = 0;		// Reset return index.
//	returnQueueStartUp[0] = 5;	// RF Mode
//	returnQueueStartUp[3] = 1;	// Autonomous mode A/B select
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//	xTimerIsTimerActive_fake.return_val = 0;
//	gpio_get_pin_value_fake.return_val = 0;
//	SatelliteOperationsTask();
//	RESET_FAKE(TimerReset);
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(gpio_set_pin_low);
//	xTimerIsTimerActive_fake.return_val = 0;
//	returnStartUpIndex = 0;		// Reset return index.
//	gpio_get_pin_value_fake.return_val = 1;
//	SatelliteOperationsTask();
//	EXPECT_EQ(1, TimerReset_fake.call_count);
//	EXPECT_EQ(1, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(3, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_low_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_low_fake.arg0_history[2]); 
//}
///***************************************************/
///* General testing.				   */
///***************************************************/
//TEST_F(SatelliteOperations, TestThatGetWODDataDoesntCrash)
//{
//	GetWODData(0,0);
//}
//
//TEST_F(SatelliteOperations, TestThatGetWODDataDoesNothingWithoutASemaphoreBeingValid)
//{
//	xSemaphoreCreateRecursiveMutexStatic_fake.return_val = 0;
//	InitialiseSatelliteOperations(0);
//	RESET_FAKE(GetCANObjectDataRaw);
//	GetWODData(0,buffer);
//	EXPECT_EQ(0, GetCANObjectDataRaw_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatGetWODDataLockAndUnlocks)
//{
//	RESET_FAKE(xSemaphoreGiveRecursive);
//	RESET_FAKE(xSemaphoreTakeRecursive);
//	GetWODData(0,buffer);
//	EXPECT_EQ(1, xSemaphoreGiveRecursive_fake.call_count);
//	EXPECT_EQ(1, xSemaphoreTakeRecursive_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatGetWODDataReturnsZerosWhenInitialised)
//{
//	GetWODData(0,buffer);
//
//	for(uint32_t i = 0; i < sizeof(buffer); i++)
//	{
//		EXPECT_EQ(0, buffer[i]);
//	}
//}
//
//TEST_F(SatelliteOperations, TestThatWodTakeSampleDoesNothingWithoutASemaphoreBeingValid)
//{
//	xSemaphoreCreateRecursiveMutexStatic_fake.return_val = 0;
//	InitialiseSatelliteOperations(0);
//	p_WodTakeSample(0);
//	RESET_FAKE(GetCANObjectDataRaw);
//	SatelliteOperationsTask();
//	EXPECT_EQ(12, GetCANObjectDataRaw_fake.call_count);
//
//}
//
//TEST_F(SatelliteOperations, TestThatWodTakeSampleLockAndUnlocks)
//{
//	InitialiseSatelliteOperations(1);
//	RESET_FAKE(xSemaphoreGiveRecursive);
//	RESET_FAKE(xSemaphoreTakeRecursive);
//	xSemaphoreTakeRecursive_fake.return_val = 1;
//	p_WodTakeSample(0);
//	SatelliteOperationsTask();
//	EXPECT_EQ(1, xSemaphoreGiveRecursive_fake.call_count);
//	EXPECT_EQ(1, xSemaphoreTakeRecursive_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatWodTakeSampleCallsTheCorrectCanODFields)
//{
//	RESET_FAKE(GetCANObjectDataRaw);
//	p_WodTakeSample(0);
//	SatelliteOperationsTask();
//	
//	/* EPS Board */
//	EXPECT_EQ(AMS_EPS_CCT_T, GetCANObjectDataRaw_fake.arg0_history[0]);		
//	EXPECT_EQ(AMS_EPS_ENC_T, GetCANObjectDataRaw_fake.arg0_history[1]);
//	EXPECT_EQ(AMS_EPS_DCDC_T, GetCANObjectDataRaw_fake.arg0_history[2]);
//	EXPECT_EQ(AMS_EPS_DCDC_I, GetCANObjectDataRaw_fake.arg0_history[3]);
//	EXPECT_EQ(AMS_EPS_DCDC_V, GetCANObjectDataRaw_fake.arg0_history[4]);
//	EXPECT_EQ(AMS_EPS_6V_V, GetCANObjectDataRaw_fake.arg0_history[5]);
//	EXPECT_EQ(AMS_EPS_9V_V, GetCANObjectDataRaw_fake.arg0_history[6]);
//	EXPECT_EQ(AMS_EPS_3V3_V, GetCANObjectDataRaw_fake.arg0_history[7]);
//	EXPECT_EQ(AMS_EPS_6V_I, GetCANObjectDataRaw_fake.arg0_history[8]);
//	EXPECT_EQ(AMS_EPS_3V3_I, GetCANObjectDataRaw_fake.arg0_history[9]);
//	EXPECT_EQ(AMS_EPS_9V_I, GetCANObjectDataRaw_fake.arg0_history[10]);
//
//	/* L-band receiver */
//	EXPECT_EQ(AMS_L_RSSI_TRANS, GetCANObjectDataRaw_fake.arg0_history[11]);
//	EXPECT_EQ(AMS_L_DOPPLER_COMMAND, GetCANObjectDataRaw_fake.arg0_history[12]);
//	EXPECT_EQ(AMS_L_RSSI_COMMAND, GetCANObjectDataRaw_fake.arg0_history[13]);
//	EXPECT_EQ(AMS_L_T, GetCANObjectDataRaw_fake.arg0_history[14]);
//
//	/* VHF transmitter */
//	EXPECT_EQ(AMS_VHF_BPSK_PA_T, GetCANObjectDataRaw_fake.arg0_history[15]);
//	EXPECT_EQ(AMS_VHF_FP, GetCANObjectDataRaw_fake.arg0_history[16]);
//	EXPECT_EQ(AMS_VHF_RP, GetCANObjectDataRaw_fake.arg0_history[17]);
//	EXPECT_EQ(AMS_VHF_BPSK_PA_I, GetCANObjectDataRaw_fake.arg0_history[18]);
//	EXPECT_EQ(AMS_VHF_FM_PA_I, GetCANObjectDataRaw_fake.arg0_history[19]);
//	EXPECT_EQ(AMS_VHF_FM_PA_T, GetCANObjectDataRaw_fake.arg0_history[20]);
//
//	/* AMSAT CCT */
//	//EXPECT_EQ(0, GetCANObjectDataRaw_fake.arg0_history[]);	// Time remaining
//	EXPECT_EQ(AMS_OBC_P_UP, GetCANObjectDataRaw_fake.arg0_history[21]);
//	EXPECT_EQ(AMS_OBC_MEM_STAT_1, GetCANObjectDataRaw_fake.arg0_history[22]);	
//	EXPECT_EQ(AMS_OBC_CAN_STATUS, GetCANObjectDataRaw_fake.arg0_history[23]);
//	EXPECT_EQ(AMS_OBC_CAN_PL, GetCANObjectDataRaw_fake.arg0_history[24]);
//	EXPECT_EQ(AMS_OBC_CAN_PL_STATUS, GetCANObjectDataRaw_fake.arg0_history[25]);
//
//	/*ESEO EPS */
//	EXPECT_EQ(PMM_TEMP_SP1_SENS_1, GetCANObjectDataRaw_fake.arg0_history[26]);
//	EXPECT_EQ(PMM_TEMP_SP2_SENS_1, GetCANObjectDataRaw_fake.arg0_history[27]);
//	EXPECT_EQ(PMM_TEMP_SP3_SENS_1, GetCANObjectDataRaw_fake.arg0_history[28]);
//	EXPECT_EQ(PMM_CURRENT_BP1, GetCANObjectDataRaw_fake.arg0_history[29]);
//
//	//EXPECT_EQ(, GetCANObjectDataRaw_fake.arg0_history[30]);	// Battery bus voltage
//	//EXPECT_EQ(, GetCANObjectDataRaw_fake.arg0_history[31]);	// Battery bus current
//
//	/* ESEO ADCS */
//	EXPECT_EQ(ACS_OMEGA_X, GetCANObjectDataRaw_fake.arg0_history[32]);	
//	EXPECT_EQ(ACS_OMEGA_Y, GetCANObjectDataRaw_fake.arg0_history[33]);	
//	EXPECT_EQ(ACS_OMEGA_Z, GetCANObjectDataRaw_fake.arg0_history[34]);
//	EXPECT_EQ(ACS_OMEGA_VX, GetCANObjectDataRaw_fake.arg0_history[35]);
//	EXPECT_EQ(ACS_OMEGA_VY, GetCANObjectDataRaw_fake.arg0_history[36]);
//	EXPECT_EQ(ACS_OMEGA_VZ, GetCANObjectDataRaw_fake.arg0_history[37]);
//	EXPECT_EQ(ACS_OMEGA_P, GetCANObjectDataRaw_fake.arg0_history[38]);
//	EXPECT_EQ(ACS_OMEGA_Q, GetCANObjectDataRaw_fake.arg0_history[39]);
//	EXPECT_EQ(ACS_OMEGA_R, GetCANObjectDataRaw_fake.arg0_history[40]);
//}
//
//TEST_F(SatelliteOperations, TestThatWODReturnsTheCorrectBitStreamForSingleWODSample)
//{
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawFAKE;
//	
//	p_WodTakeSample(0);
//	SatelliteOperationsTask();
//
//	GetWODData(0,buffer);
//
//	for(uint32_t i = 0; i < 39; i++)
//	{
//		EXPECT_EQ(incrementingWOD[i], buffer[i]);
//	}
//}
//
//TEST_F(SatelliteOperations, TestThatWODReturnsCorrectSamples)
//{
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawFAKE2;
//
//	p_WodTakeSample(0);
//	SatelliteOperationsTask();
//	p_WodTakeSample(0);
//	SatelliteOperationsTask();
//	p_WodTakeSample(0);
//	SatelliteOperationsTask();
//	p_WodTakeSample(0);
//	SatelliteOperationsTask();
//	p_WodTakeSample(0);
//	SatelliteOperationsTask();
//
//	GetWODData(0,buffer);
//	EXPECT_EQ(4, buffer[0]);
//	EXPECT_EQ(0, buffer[39]);
//	EXPECT_EQ(3, buffer[40]);
//	EXPECT_EQ(0, buffer[79]);
//	EXPECT_EQ(2, buffer[80]);
//	EXPECT_EQ(0, buffer[119]);
//	EXPECT_EQ(1, buffer[120]);
//	EXPECT_EQ(0, buffer[159]);
//	EXPECT_EQ(0, buffer[160]);
//}
//
//TEST_F(SatelliteOperations, TestThatWODReturnsCorrectSamplesForAll20Frames)
//{
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawFAKE2;
//
//	for(uint32_t i = 0; i < 100; i++)
//	{
//		p_WodTakeSample(0);
//		SatelliteOperationsTask();
//	}
//
//	for(uint32_t i = 0; i < 20; i++)
//	{
//		GetWODData(i,buffer);
//		EXPECT_EQ(100-(i*5)-1, buffer[0]);
//		EXPECT_EQ(100-(i*5)-2, buffer[40]);
//		EXPECT_EQ(100-(i*5)-3, buffer[80]);
//		EXPECT_EQ(100-(i*5)-4, buffer[120]);
//		EXPECT_EQ(100-(i*5)-5, buffer[160]);
//	}
//}
//
//TEST_F(SatelliteOperations, TestThatWODReturnsCorrectSamplesForAll20FramesAfterAbritaryTime)
//{
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawFAKE2;
//
//	for(uint32_t i = 0; i < 1234; i++)
//	{
//		p_WodTakeSample(0);
//		SatelliteOperationsTask();
//	}
//
//	callCount = 0;
//	wodSampleCount = 0;
//
//	for(uint32_t i = 0; i < 100; i++)
//	{
//		p_WodTakeSample(0);
//		SatelliteOperationsTask();
//	}
//
//	for(uint32_t i = 0; i < 20; i++)
//	{
//		GetWODData(i,buffer);
//		EXPECT_EQ(100-(i*5)-1, buffer[0]);
//		EXPECT_EQ(100-(i*5)-2, buffer[40]);
//		EXPECT_EQ(100-(i*5)-3, buffer[80]);
//		EXPECT_EQ(100-(i*5)-4, buffer[120]);
//		EXPECT_EQ(100-(i*5)-5, buffer[160]);
//	}
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationWritesToFlashIfWeDontHaveValidData)
//{
//	p_payloadDataStore[0].dataInvalid = 0xFF;
//	p_payloadDataStore[0].dataValid = 0xFF;
//	p_payloadDataStore[1].dataInvalid = 0xFF;
//	p_payloadDataStore[1].dataValid = 0xFF;
//	RESET_FAKE(ClearPage);
//	RESET_FAKE(ClearPageBuffer);
//	RESET_FAKE(WriteToFlash);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(4, ClearPageBuffer_fake.call_count);
//	EXPECT_EQ(4, ClearPage_fake.call_count);
//	EXPECT_EQ(4, WriteToFlash_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationGetsDefaultsFromCANOD)
//{
//	p_payloadDataStore[0].dataInvalid = 0xFF;
//	p_payloadDataStore[0].dataValid = 0xFF;
//	p_payloadDataStore[1].dataInvalid = 0xFF;
//	p_payloadDataStore[1].dataValid = 0xFF;
//	RESET_FAKE(ClearPage);
//	RESET_FAKE(ClearPageBuffer);
//	RESET_FAKE(WriteToFlash);
//	RESET_FAKE(GetCANObjectDataRaw);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(16, GetCANObjectDataRaw_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationWritesToBothFlashRegionsAndMarksAsSuccessfull)
//{
//	p_payloadDataStore[0].dataInvalid = 0xFF;
//	p_payloadDataStore[0].dataValid = 0xFF;
//	p_payloadDataStore[1].dataInvalid = 0xFF;
//	p_payloadDataStore[1].dataValid = 0xFF;
//	RESET_FAKE(ClearPage);
//	RESET_FAKE(ClearPageBuffer);
//	RESET_FAKE(WriteToFlash);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(4, ClearPageBuffer_fake.call_count);
//	EXPECT_EQ(4, ClearPage_fake.call_count);
//	EXPECT_EQ(4, WriteToFlash_fake.call_count);
//	EXPECT_EQ(255, p_payloadDataStore[0].dataInvalid);
//	EXPECT_NE(0, p_payloadDataStore[0].dataValid);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationDoesntGetDefaultsFromCANODorWritesToFlashIfWeHaveValidData)
//{
//	p_payloadDataStore[0].dataInvalid = 0x0;
//	p_payloadDataStore[0].dataValid = 0x1;
//	p_payloadDataStore[1].dataInvalid = 0x0;
//	p_payloadDataStore[1].dataValid = 0x0;
//	RESET_FAKE(ClearPage);
//	RESET_FAKE(ClearPageBuffer);
//	RESET_FAKE(WriteToFlash);
//	RESET_FAKE(GetCANObjectDataRaw);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(0, ClearPageBuffer_fake.call_count);
//	EXPECT_EQ(0, ClearPage_fake.call_count);
//	EXPECT_EQ(0, WriteToFlash_fake.call_count);
//	EXPECT_EQ(4, GetCANObjectDataRaw_fake.call_count);
//
//	p_payloadDataStore[0].dataInvalid = 0x0;
//	p_payloadDataStore[0].dataValid = 0x0;
//	p_payloadDataStore[1].dataInvalid = 0x0;
//	p_payloadDataStore[1].dataValid = 0x1;
//	RESET_FAKE(ClearPage);
//	RESET_FAKE(ClearPageBuffer);
//	RESET_FAKE(WriteToFlash);
//	RESET_FAKE(GetCANObjectDataRaw);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(0, ClearPageBuffer_fake.call_count);
//	EXPECT_EQ(0, ClearPage_fake.call_count);
//	EXPECT_EQ(0, WriteToFlash_fake.call_count);
//	EXPECT_EQ(4, GetCANObjectDataRaw_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationPassesCorrectValuesToCANGet)
//{
//	p_payloadDataStore[0].dataInvalid = 0x0;
//	p_payloadDataStore[0].dataValid = 0x0;
//	p_payloadDataStore[1].dataInvalid = 0x0;
//	p_payloadDataStore[1].dataValid = 0x0;
//	RESET_FAKE(ClearPage);
//	RESET_FAKE(ClearPageBuffer);
//	RESET_FAKE(WriteToFlash);
//	RESET_FAKE(GetCANObjectDataRaw);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(AMS_OBC_RFMODE_DEF ,GetCANObjectDataRaw_fake.arg0_history[0]);
//	EXPECT_EQ(AMS_OBC_DATAMODE_DEF ,GetCANObjectDataRaw_fake.arg0_history[1]);
//	EXPECT_EQ(AMS_OBC_ECLIPSE_THRESH ,GetCANObjectDataRaw_fake.arg0_history[2]);
//	EXPECT_EQ(AMS_OBC_ECLIPSE_MODE_DEF ,GetCANObjectDataRaw_fake.arg0_history[3]);
//	EXPECT_EQ(AMS_OBC_SEQNUM ,GetCANObjectDataRaw_fake.arg0_history[4]);
//	EXPECT_EQ(AMS_OBC_FLASH_RATE ,GetCANObjectDataRaw_fake.arg0_history[5]);
//	EXPECT_EQ(AMS_OBC_RFMODE_DEF ,GetCANObjectDataRaw_fake.arg0_history[6]);
//	EXPECT_EQ(AMS_OBC_DATAMODE_DEF ,GetCANObjectDataRaw_fake.arg0_history[7]);
//	EXPECT_EQ(AMS_OBC_ECLIPSE_THRESH ,GetCANObjectDataRaw_fake.arg0_history[8]);
//	EXPECT_EQ(AMS_OBC_ECLIPSE_MODE_DEF ,GetCANObjectDataRaw_fake.arg0_history[9]);
//	EXPECT_EQ(AMS_OBC_SEQNUM ,GetCANObjectDataRaw_fake.arg0_history[10]);
//	EXPECT_EQ(AMS_OBC_FLASH_RATE ,GetCANObjectDataRaw_fake.arg0_history[11]);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationCallsCANSet)
//{
//	p_payloadDataStore[0].rfMode = 0x01;
//	p_payloadDataStore[0].dataMode = 0x02;
//	p_payloadDataStore[0].eclipseThresholdValue = 0x03;
//	p_payloadDataStore[0].autonomousModeDefault = 0x04;
//	p_payloadDataStore[0].sequenceNumber = 0x05;
//	p_payloadDataStore[0].flashUpdateRate = 0x06;
//	p_payloadDataStore[0].dataInvalid = 0x0;
//	p_payloadDataStore[0].dataValid = 0x1;
//
//	RESET_FAKE(ClearPage);
//	RESET_FAKE(ClearPageBuffer);
//	RESET_FAKE(WriteToFlash);
//	RESET_FAKE(SetCANObjectDataRaw);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(6, SetCANObjectDataRaw_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationPassesCorrectIDValuesToCANSet)
//{
//	p_payloadDataStore[0].rfMode = 0x01;
//	p_payloadDataStore[0].dataMode = 0x02;
//	p_payloadDataStore[0].eclipseThresholdValue = 0x03;
//	p_payloadDataStore[0].autonomousModeDefault = 0x04;
//	p_payloadDataStore[0].sequenceNumber = 0x05;
//	p_payloadDataStore[0].flashUpdateRate = 0x06;
//	p_payloadDataStore[0].dataInvalid = 0x0;
//	p_payloadDataStore[0].dataValid = 0x1;
//
//	RESET_FAKE(ClearPage);
//	RESET_FAKE(ClearPageBuffer);
//	RESET_FAKE(WriteToFlash);
//	RESET_FAKE(SetCANObjectDataRaw);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(AMS_OBC_RFMODE_DEF ,SetCANObjectDataRaw_fake.arg0_history[0]);
//	EXPECT_EQ(AMS_OBC_DATAMODE_DEF ,SetCANObjectDataRaw_fake.arg0_history[1]);
//	EXPECT_EQ(AMS_OBC_ECLIPSE_THRESH ,SetCANObjectDataRaw_fake.arg0_history[2]);
//	EXPECT_EQ(AMS_OBC_ECLIPSE_MODE_DEF ,SetCANObjectDataRaw_fake.arg0_history[3]);
//	EXPECT_EQ(AMS_OBC_SEQNUM ,SetCANObjectDataRaw_fake.arg0_history[4]);
//	EXPECT_EQ(AMS_OBC_FLASH_RATE ,SetCANObjectDataRaw_fake.arg0_history[5]);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationPassesCorrectDataValuesToCANSet)
//{
//	p_payloadDataStore[0].rfMode = 0x01;
//	p_payloadDataStore[0].dataMode = 0x02;
//	p_payloadDataStore[0].eclipseThresholdValue = 0x03;
//	p_payloadDataStore[0].autonomousModeDefault = 0x04;
//	p_payloadDataStore[0].sequenceNumber = 0x05;
//	p_payloadDataStore[0].flashUpdateRate = 31;
//	p_payloadDataStore[0].dataInvalid = 0x0;
//	p_payloadDataStore[0].dataValid = 0x1;
//
//	RESET_FAKE(ClearPage);
//	RESET_FAKE(ClearPageBuffer);
//	RESET_FAKE(WriteToFlash);
//	RESET_FAKE(SetCANObjectDataRaw);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(1 ,SetCANObjectDataRaw_fake.arg1_history[0]);
//	EXPECT_EQ(2 ,SetCANObjectDataRaw_fake.arg1_history[1]);
//	EXPECT_EQ(3 ,SetCANObjectDataRaw_fake.arg1_history[2]);
//	EXPECT_EQ(4 ,SetCANObjectDataRaw_fake.arg1_history[3]);
//	EXPECT_EQ(20 ,SetCANObjectDataRaw_fake.arg1_history[4]);
//	EXPECT_EQ(31 ,SetCANObjectDataRaw_fake.arg1_history[5]);
//}
//
//TEST_F(SatelliteOperations, TestThatInitialisationIncrementsSequenceNumberCorrectlyAtDefaultValue)
//{
//	p_payloadDataStore[0].sequenceNumber = 0x05;
//	p_payloadDataStore[0].flashUpdateRate = 30;			// 30 mins push to flash.
//
//	RESET_FAKE(SetCANObjectDataRaw);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(20 ,SetCANObjectDataRaw_fake.arg1_history[4]);		// Sequence number
//	EXPECT_EQ(30 ,SetCANObjectDataRaw_fake.arg1_history[5]);		// Flash update rate
//}
//
//TEST_F(SatelliteOperations, TestThatMinimumFlashUpdateRateIsLimitedTo30mins)
//{
//	p_payloadDataStore[0].sequenceNumber = 0x05;
//	p_payloadDataStore[0].flashUpdateRate = 30;			// 30 mins push to flash.
//
//	RESET_FAKE(SetCANObjectDataRaw);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(30 ,SetCANObjectDataRaw_fake.arg1_history[5]);		// Flash update rate
//}
//
//TEST_F(SatelliteOperations, TestThatMaximumFlashUpdateRateIsLimitedTo180mins)
//{
//	p_payloadDataStore[0].sequenceNumber = 0x05;
//	p_payloadDataStore[0].flashUpdateRate = 181;			// 30 mins push to flash.
//
//	RESET_FAKE(SetCANObjectDataRaw);
//	InitialiseSatelliteOperations(1);
//
//	EXPECT_EQ(180 ,SetCANObjectDataRaw_fake.arg1_history[5]);		// Flash update rate
//}
//
//TEST_F(SatelliteOperations, TestThatUpdatingDefaultValueWriteCorrectValuesToFlash)
//{
//	p_payloadDataStore[0].dataInvalid = 0x0;
//	p_payloadDataStore[0].dataValid = 0x1;
//
//	RESET_FAKE(ClearPage);
//	RESET_FAKE(ClearPageBuffer);
//	RESET_FAKE(WriteToFlash);
//	RESET_FAKE(SetCANObjectDataRaw);
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawCount;
//	p_updateDefaultValues(0);
//	SatelliteOperationsTask();
//
//	EXPECT_EQ(1, p_payloadDataStore[0].rfMode);
//	EXPECT_EQ(2, p_payloadDataStore[0].dataMode);
//	EXPECT_EQ(3, p_payloadDataStore[0].eclipseThresholdValue);
//	EXPECT_EQ(4, p_payloadDataStore[0].autonomousModeDefault);
//	EXPECT_EQ(5, p_payloadDataStore[0].sequenceNumber);
//	EXPECT_EQ(6, p_payloadDataStore[0].flashUpdateRate);
//}
//
//OBJECT_DICTIONARY_ERROR GetCANObjectDataRawCount( uint32_t z_index, uint32_t * z_data)
//{
//	count ++;
//	*z_data = count;
//	return NO_ERROR_OBJECT_DICTIONARY;
//}
//
//TEST_F(SatelliteOperations, TestThatUpdatingDefaultValueIncrementsCountInFlash)
//{
//	p_payloadDataStore[0].dataInvalid = 0x0;
//	p_payloadDataStore[0].dataValid = 0x1;
//	p_payloadDataStore[1].dataInvalid = 0x0;
//	p_payloadDataStore[1].dataValid = 0x1;
//
//	RESET_FAKE(ClearPage);
//	RESET_FAKE(ClearPageBuffer);
//	RESET_FAKE(WriteToFlash);
//	RESET_FAKE(SetCANObjectDataRaw);
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawCount;
//	p_updateDefaultValues(0);
//	SatelliteOperationsTask();
//
//	EXPECT_EQ(1, p_payloadDataStore[0].dataInvalid);
//	EXPECT_EQ(2, p_payloadDataStore[0].dataValid);
//	EXPECT_EQ(1, p_payloadDataStore[1].dataInvalid);
//	EXPECT_EQ(2, p_payloadDataStore[1].dataValid);
//}
//
//TEST_F(SatelliteOperations, TestThatUpdateDefaultValuesLocksAndUnlocks)
//{
//	RESET_FAKE(xSemaphoreGiveRecursive);
//	RESET_FAKE(xSemaphoreTakeRecursive);
//	xSemaphoreTakeRecursive_fake.return_val = 1;
//	p_updateDefaultValues(0);
//	SatelliteOperationsTask();
//	EXPECT_EQ(2, xSemaphoreGiveRecursive_fake.call_count);
//	EXPECT_EQ(2, xSemaphoreTakeRecursive_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatUpdateSequenceNumberUpdatesToTheCanOD)
//{
//	RESET_FAKE(GetCANObjectDataRaw);
//	RESET_FAKE(SetCANObjectDataRaw);
//	
//	p_sampleSequenceNumber(0);
//	SatelliteOperationsTask();
//
//	EXPECT_EQ(13, GetCANObjectDataRaw_fake.call_count);
//	EXPECT_EQ(1, xQueueSend_fake.call_count);
//}
//
//TEST_F(SatelliteOperations, TestThatUpdateSequenceNumberUpdatesToTheCanODInTheCorrectField)
//{
//	RESET_FAKE(GetCANObjectDataRaw);
//	RESET_FAKE(SetCANObjectDataRaw);
//	
//	p_sampleSequenceNumber(0);
//	SatelliteOperationsTask();
//
//	EXPECT_EQ(AMS_OBC_SEQNUM, GetCANObjectDataRaw_fake.arg0_history[0]);
//}
//
//TEST_F(SatelliteOperations, TestThatUpdateSequenceNumberIncrementsTheSequenceNumber)
//{
//	RESET_FAKE(GetCANObjectDataRaw);
//	RESET_FAKE(SetCANObjectDataRaw);
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturn5;
//	xQueueSend_fake.custom_fake = xQueueSendFAKE;
//
//	p_sampleSequenceNumber(0);
//	SatelliteOperationsTask();
//
//
//	EXPECT_EQ(0, outputBuffer[0]);
//	EXPECT_EQ(5, outputBuffer[1]);
//	EXPECT_EQ(AMS_OBC_SEQNUM, outputBuffer[2]);
//	EXPECT_EQ(6, outputBuffer[3]);
//	EXPECT_EQ(0, outputBuffer[4]);
//	EXPECT_EQ(0, outputBuffer[5]);
//}
//
///* Temperature shut down testing */
//TEST_F(SatelliteOperations, TestThatSafeModeIsSwitchedToWhenSafeModeEnabledAndOverTemperature)
//{
//	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
//	p_payloadDataStore[0].dataInvalid = 0;
//	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
//	returnQueueStartUp[0] = 1;	// RF Mode
//	returnQueueStartUp[1] = 0;	// Data mode
//	returnQueueStartUp[9] = 0;	// Safe mode enabled
//	returnQueueStartUp[10] = 0;	// Safe mode active
//	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
//
//	RESET_FAKE(gpio_configure_pin);
//	InitialiseSatelliteOperations(1);
//	returnQueueStartUp[0] = 4;	// RF Mode
//	RESET_FAKE(gpio_configure_pin);
//	RESET_FAKE(gpio_set_pin_low);
//	RESET_FAKE(gpio_set_pin_high);
//	returnStartUpIndex = 0;
//	SatelliteOperationsTask();
//
//	/* In HP transponder */
//	EXPECT_EQ(3, gpio_set_pin_high_fake.call_count);
//	EXPECT_EQ(1, gpio_set_pin_low_fake.call_count);
//	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
//	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_high_fake.arg0_history[1]); 
//	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_high_fake.arg0_history[2]); 
//
//	returnQueueStartUp[9] = 1;		// Safe mode enabled
//	returnQueueStartUp[10] = 0;		// Safe mode active	
//	returnQueueStartUp[11] = 58;	// Temperature (59 is limit)
//	RESET_FAKE(gpio_configure_pin);
//	RESET_FAKE(gpio_set_pin_low);
//	RESET_FAKE(gpio_set_pin_high);
//	RESET_FAKE(ChangeMode);
//	RESET_FAKE(ChangeDownlinkMode);
//	RESET_FAKE(SetCANObjectDataRaw);
//	returnStartUpIndex = 0;
//	SatelliteOperationsTask();
//
//	EXPECT_EQ(MODE_1K2_LOW_POWER, ChangeMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeMode_fake.call_count);
//	EXPECT_EQ(MODE_1K2, ChangeDownlinkMode_fake.arg0_history[0]);
//	EXPECT_EQ(1, ChangeDownlinkMode_fake.call_count);
//	EXPECT_EQ(1, SetCANObjectDataRaw_fake.call_count);
//}
//
//
/////* Temperature shut down testing */
////TEST_F(SatelliteOperations, TestThatSafeModeIsSwitchedToWhenSafeModeEnabledAndOverTemperatureForReceiveOnly)
////{
////	p_payloadDataStore[0].dataValid = 1;					// So we don't get from the OD for default values.
////	p_payloadDataStore[0].dataInvalid = 0;
////	GetCANObjectDataRaw_fake.custom_fake = &GetCANObjectDataRawReturnStartUp;
////	returnQueueStartUp[0] = 1;	// RF Mode
////	returnQueueStartUp[1] = 0;	// Data mode
////	returnQueueStartUp[9] = 0;	// Safe mode enabled
////	returnQueueStartUp[10] = 0;	// Safe mode active
////	returnQueueStartUp[11] = 59;	// Temperature (59 is limit)
////
////	RESET_FAKE(gpio_configure_pin);
////	InitialiseSatelliteOperations(1);
////	returnQueueStartUp[0] = 4;	// RF Mode
////	RESET_FAKE(gpio_configure_pin);
////	RESET_FAKE(gpio_set_pin_low);
////	RESET_FAKE(gpio_set_pin_high);
////	returnStartUpIndex = 0;
////	SatelliteOperationsTask();
////
////	/* In HP transponder */
////	EXPECT_EQ(3, gpio_set_pin_high_fake.call_count);
////	EXPECT_EQ(1, gpio_set_pin_low_fake.call_count);
////	EXPECT_EQ(BPSK_ON_OFF_SWITCH, gpio_set_pin_low_fake.arg0_history[0]); 
////	EXPECT_EQ(TRANSPONDER_ON_OFF_SWITCH, gpio_set_pin_high_fake.arg0_history[0]); 
////	EXPECT_EQ(TRANSPONDER_POWER_SWITCH, gpio_set_pin_high_fake.arg0_history[1]); 
////	EXPECT_EQ(ANTENNA_SWITCH, gpio_set_pin_high_fake.arg0_history[2]); 
////
////	returnQueueStartUp[9] = 1;		// Safe mode enabled
////	returnQueueStartUp[10] = 0;		// Safe mode active	
////	returnQueueStartUp[11] = 52;	// Temperature (59 is limit)
////	RESET_FAKE(gpio_configure_pin);
////	RESET_FAKE(gpio_set_pin_low);
////	RESET_FAKE(gpio_set_pin_high);
////	RESET_FAKE(ChangeMode);
////	RESET_FAKE(ChangeDownlinkMode);
////	RESET_FAKE(SetCANObjectDataRaw);
////	returnStartUpIndex = 0;
////	SatelliteOperationsTask();
////
////	EXPECT_EQ(2, gpio_set_pin_low_fake.call_count);
////	EXPECT_EQ(2, gpio_set_pin_high_fake.call_count);
////}
