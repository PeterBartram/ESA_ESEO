#include "gtest/gtest.h"

extern "C"
{
	#include "ADCDriver.h"
	#include "fff.h"
	#include "Fake_TwoWireInterface.h"
	#include "Diagnostics.h"
	#include "uc3c0512c.h"
	#include "gpio.h"
	#include "Fake_gpio.h"
}

DEFINE_FFF_GLOBALS;

status_code_t twim_write_packet_custom_fake(volatile avr32_twim_t *, const twim_package_t *);
status_code_t twim_read_packet_custom_fake(volatile avr32_twim_t *, const twim_package_t *);
status_code_t twim_read_packet_custom_fake_return_data(volatile avr32_twim_t * z_twim, const twim_package_t * z_twimPacket);
status_code_t twim_read_packet_custom_fake_return_fixed_data(volatile avr32_twim_t * z_twim, const twim_package_t * z_twimPacket);

/* Local testing global variables. */
twim_package_t twimPackageBuffer[20];
uint32_t twimBufferCount;
volatile avr32_twim_t * twimI2CBuffer[20];

class DiagnosticsADCs : public testing::Test
{
public:
	uint16_t buffer[50];
	uint32_t bufferSize;

	void SetUp()
	{
		bufferSize = 50;
	}

	virtual void TearDown() 
	{
		RESET_FAKE(gpio_enable_module);
		RESET_FAKE(twim_master_init);
		RESET_FAKE(twim_write_packet);
		RESET_FAKE(twim_read_packet);
		

		for(uint32_t i=0; i < sizeof(buffer)/sizeof(buffer[0]); i++)
		{
			buffer[i] = 0;
		}
		twimBufferCount = 0;
		for(uint32_t i=0; i < sizeof(twimPackageBuffer)/sizeof(twimPackageBuffer[0]); i++)
		{
			twimPackageBuffer[i].addr[0] = 0;
			twimPackageBuffer[i].addr[1] = 0;
			twimPackageBuffer[i].addr[2] = 0;
			twimPackageBuffer[i].addr_length = 0;
			twimPackageBuffer[i].buffer = 0;
			twimPackageBuffer[i].chip = 0;
			twimPackageBuffer[i].length = 0;
			twimPackageBuffer[i].no_wait = 0;
			twimI2CBuffer[i] = 0;
		}
	}
};

/***************************************************/
/* General initialisation testing.				   */
/***************************************************/
TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsReturnsErrorIfNoBufferPassedIn)
{
	EXPECT_EQ(ADCInitialise(0, bufferSize), NO_BUFFER_PROVIDED_ADC);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsReturnsErrorIfNoBufferSizePassedIn)
{
	EXPECT_EQ(ADCInitialise(buffer, 0), NO_BUFFER_PROVIDED_ADC);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsReturnsOkWhenPassedTheCorrectBufferParameters)
{
	EXPECT_EQ(ADCInitialise(buffer, bufferSize), NO_ERROR_ADC);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCCalls_gpio_enable_module)
{
	ADCInitialise(buffer, bufferSize);
	EXPECT_EQ(gpio_enable_module_fake.call_count, 1);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCReturnsErrorIf_gpio_enable_module_fails)
{
	gpio_enable_module_fake.return_val = 1;
	EXPECT_NE(ADCInitialise(buffer, bufferSize), 0);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsPasses_gpio_enable_module_the_TheCorrectI2CBus)
{
	ADCInitialise(buffer, bufferSize);
	gpio_map_t_intermideary * gpio_enable_module_argment_pointer = (gpio_map_t_intermideary *)gpio_enable_module_fake.arg0_val;

	EXPECT_EQ(gpio_enable_module_argment_pointer[0].function, AVR32_TWIMS0_TWCK_0_0_FUNCTION);
	EXPECT_EQ(gpio_enable_module_argment_pointer[0].pin, AVR32_TWIMS0_TWCK_0_0_PIN);
	EXPECT_EQ(gpio_enable_module_argment_pointer[1].function, AVR32_TWIMS0_TWCK_0_0_FUNCTION);
	EXPECT_EQ(gpio_enable_module_argment_pointer[1].pin, AVR32_TWIMS0_TWD_0_0_PIN);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsCalls_gpio_enable_module_WithTheCorrectSizeConfigurationStructure)
{
	ADCInitialise(buffer, bufferSize);
	EXPECT_EQ(gpio_enable_module_fake.arg1_val, 2);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsCalls_twim_master_init)
{
	ADCInitialise(buffer, bufferSize);
	EXPECT_EQ(twim_master_init_fake.call_count, 1);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsReturnErrorIf_twim_master_init_returnsError)
{
	twim_master_init_fake.return_val = (status_code_t)1;
	EXPECT_EQ(ADCInitialise(buffer, bufferSize), COULD_NOT_INITIALISE_ADC);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsPasses_twim_master_init_TheCorrectI2CModule)
{
	ADCInitialise(buffer, bufferSize);
	EXPECT_EQ(twim_master_init_fake.arg0_val, &AVR32_TWIM0);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsPasses_twim_master_init_TheCorrectTwimChipAddress)
{
	ADCInitialise(buffer, bufferSize);
	twi_options_t * result = (twi_options_t *)twim_master_init_fake.arg1_val;
	EXPECT_EQ(result->chip, 0x35);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsPasses_twim_master_init_TheCorrectTwimPBAClockSpeed)
{
	ADCInitialise(buffer, bufferSize);
	twi_options_t * result = (twi_options_t *)twim_master_init_fake.arg1_val;
	EXPECT_EQ(result->pba_hz, 60000000);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsPasses_twim_master_init_TheCorrectSMBusInitialisationValue)
{
	ADCInitialise(buffer, bufferSize);
	twi_options_t * result = (twi_options_t *)twim_master_init_fake.arg1_val;
	EXPECT_EQ(result->smbus, 0);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsPasses_twim_master_init_TheCorrectTwimSpeed)
{
	ADCInitialise(buffer, bufferSize);
	twi_options_t * result = (twi_options_t *)twim_master_init_fake.arg1_val;
	EXPECT_EQ(result->speed, 100000);
}

/* Sending of initialization packets over the bus. */
TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsCallsTwimWritePacketForReceiverADC)
{
	ADCInitialise(buffer, bufferSize);
	EXPECT_EQ(twim_write_packet_fake.call_count, 3);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsReturnNoErrorIfTwimWritePacketIsSuccessful)
{
	twim_write_packet_fake.return_val = (status_code_t)0;
	EXPECT_EQ(ADCInitialise(buffer, bufferSize), NO_ERROR_ADC);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsReturnErrorIfTwimWritePacketReturnsError)
{
	twim_write_packet_fake.return_val = (status_code_t)1;
	EXPECT_NE(ADCInitialise(buffer, bufferSize), NO_ERROR_ADC);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsCalls_twim_write_packet_WithCorrectConfigurationDataForADC1)
{
	twim_write_packet_fake.custom_fake = twim_write_packet_custom_fake;
	ADCInitialise(buffer, bufferSize);
	EXPECT_EQ(twimPackageBuffer[0].chip, 0x34);		//LBAND receiver address.
	EXPECT_EQ(twimPackageBuffer[0].addr[0], 0x82);	
	EXPECT_EQ(twimPackageBuffer[0].addr[1], 0x7);
	EXPECT_EQ(twimPackageBuffer[0].addr_length, 2);
	EXPECT_EQ(twimPackageBuffer[0].buffer, (void*)0);
	EXPECT_EQ(twimPackageBuffer[0].length, 0);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsCalls_twim_write_packet_WithCorrectConfigurationDataForADC2)
{
	twim_write_packet_fake.custom_fake = twim_write_packet_custom_fake;
	ADCInitialise(buffer, bufferSize);
	EXPECT_EQ(twimPackageBuffer[1].chip, 0x33);		//BPSK receiver address.
	EXPECT_EQ(twimPackageBuffer[1].addr[0], 0x82);	
	EXPECT_EQ(twimPackageBuffer[1].addr[1], 0xF);
	EXPECT_EQ(twimPackageBuffer[1].addr_length, 2);
	EXPECT_EQ(twimPackageBuffer[1].buffer, (void*)0);
	EXPECT_EQ(twimPackageBuffer[1].length, 0);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsCalls_twim_write_packet_WithCorrectConfigurationDataForADC3)
{
	twim_write_packet_fake.custom_fake = twim_write_packet_custom_fake;
	ADCInitialise(buffer, bufferSize);
	EXPECT_EQ(twimPackageBuffer[2].chip, 0x35);		//EPS receiver address.
	EXPECT_EQ(twimPackageBuffer[2].addr[0], 0x82);	
	EXPECT_EQ(twimPackageBuffer[2].addr[1], 0x15);
	EXPECT_EQ(twimPackageBuffer[2].addr_length, 2);
	EXPECT_EQ(twimPackageBuffer[2].buffer, (void*)0);
	EXPECT_EQ(twimPackageBuffer[2].length, 0);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsReturnErrorIfTwimWritePacketToSecondADCReturnsError)
{
	status_code_t testReturnValues[3] = {(status_code_t)0,(status_code_t)1,(status_code_t)0};
	SET_RETURN_SEQ(twim_write_packet, testReturnValues, 3);
	EXPECT_NE(ADCInitialise(buffer, bufferSize), NO_ERROR_ADC);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsReturnErrorIfTwimWritePacketToThirdADCReturnsError)
{
	status_code_t testReturnValues[3] = {(status_code_t)0,(status_code_t)0,(status_code_t)1};
	SET_RETURN_SEQ(twim_write_packet, testReturnValues, 3);
	EXPECT_NE(ADCInitialise(buffer, bufferSize), NO_ERROR_ADC);
}

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsCalls_twim_write_packet_WithCorrectI2CBus)
{
	twim_write_packet_fake.custom_fake = twim_write_packet_custom_fake;
	ADCInitialise(buffer, bufferSize);
	EXPECT_EQ(twimI2CBuffer[0], &AVR32_TWIM0);
}

status_code_t twim_write_packet_custom_fake(volatile avr32_twim_t * z_twim, const twim_package_t * z_twimPacket)
{
	twimI2CBuffer[twimBufferCount] = z_twim;
	twimPackageBuffer[twimBufferCount].addr[0] = z_twimPacket->addr[0];
	twimPackageBuffer[twimBufferCount].addr[1] = z_twimPacket->addr[1];
	twimPackageBuffer[twimBufferCount].addr[2] = z_twimPacket->addr[2];
	twimPackageBuffer[twimBufferCount].addr_length = z_twimPacket->addr_length;
	twimPackageBuffer[twimBufferCount].buffer = z_twimPacket->buffer;
	twimPackageBuffer[twimBufferCount].chip = z_twimPacket->chip;
	twimPackageBuffer[twimBufferCount].length = z_twimPacket->length;
	twimPackageBuffer[twimBufferCount].no_wait = z_twimPacket->no_wait;
	twimBufferCount++;
	return STATUS_OK;
}

/*************************/
/* FetchDiagnstics data. */
/*************************/

TEST_F(DiagnosticsADCs, TestThatFetchDiagnosticsDataCalls_twim_read_packet)
{
	ADCInitialise(buffer, bufferSize);
	FetchDiagnosticsData();
	EXPECT_EQ(twim_read_packet_fake.call_count, 3);
}

TEST_F(DiagnosticsADCs, TestThatFetchDiagnosticsDataReturnsErrorIf_twim_read_packet_fails)
{
	twim_read_packet_fake.return_val = (status_code_t)1;
	ADCInitialise(buffer, bufferSize);
	EXPECT_NE(FetchDiagnosticsData(), NO_ERROR_ADC);
}

TEST_F(DiagnosticsADCs, TestThatADCReadReturnsNotInitialisedIfWeHaveUninitialised)
{
	ADCInitialise(buffer, bufferSize);
	ADCUninitialise();
	EXPECT_NE(FetchDiagnosticsData(), NO_ERROR_ADC);
}

TEST_F(DiagnosticsADCs, TestThatFetchDiagnosticsDataReturnsErrorIf_twim_read_packet_failsForSecondADC)
{
	status_code_t testReturnValues[3] = {(status_code_t)0,(status_code_t)1,(status_code_t)0};
	SET_RETURN_SEQ(twim_read_packet, testReturnValues, 3);
	ADCInitialise(buffer, bufferSize);
	EXPECT_NE(FetchDiagnosticsData(), NO_ERROR_ADC);
}

TEST_F(DiagnosticsADCs, TestThatFetchDiagnosticsDataReturnsErrorIf_twim_read_packet_failsForThirdADC)
{
	status_code_t testReturnValues[3] = {(status_code_t)0,(status_code_t)0,(status_code_t)1};
	SET_RETURN_SEQ(twim_read_packet, testReturnValues, 3);
	ADCInitialise(buffer, bufferSize);
	EXPECT_NE(FetchDiagnosticsData(), NO_ERROR_ADC);
}

TEST_F(DiagnosticsADCs, TestThatFetchDiagnosticsDataADCsCalls_twim_read_packetWithCorrectInputDataForADC1)
{
	twim_read_packet_fake.custom_fake = twim_read_packet_custom_fake;
	ADCInitialise(buffer, bufferSize);
	FetchDiagnosticsData();
	EXPECT_EQ(twimPackageBuffer[0].chip, 0x34);
	EXPECT_EQ(twimPackageBuffer[0].addr_length, 0);
	EXPECT_NE(twimPackageBuffer[0].buffer, (void*)0);
	EXPECT_EQ(twimPackageBuffer[0].length, 8);	
}

TEST_F(DiagnosticsADCs, TestThatFetchDiagnosticsDataADCsCalls_twim_read_packetWithCorrectInputDataForADC2)
{
	twim_read_packet_fake.custom_fake = twim_read_packet_custom_fake;
	ADCInitialise(buffer, bufferSize);
	FetchDiagnosticsData();
	EXPECT_EQ(twimPackageBuffer[1].chip, 0x33);
	EXPECT_EQ(twimPackageBuffer[1].addr_length, 0);
	EXPECT_NE(twimPackageBuffer[1].buffer, (void*)0);
	EXPECT_EQ(twimPackageBuffer[1].length, 16);	
}								

TEST_F(DiagnosticsADCs, TestThatFetchDiagnosticsDataADCsCalls_twim_read_packetWithCorrectInputDataForADC3)
{
	twim_read_packet_fake.custom_fake = twim_read_packet_custom_fake;
	ADCInitialise(buffer, bufferSize);
	FetchDiagnosticsData();
	EXPECT_EQ(twimPackageBuffer[2].chip, 0x35);
	EXPECT_EQ(twimPackageBuffer[2].addr_length, 0);
	EXPECT_NE(twimPackageBuffer[2].buffer, (void*)0);
	EXPECT_EQ(twimPackageBuffer[2].length, 24);	
}								

TEST_F(DiagnosticsADCs, TestThatInitialiseDiagnosticsADCsCalls_twim_read_packet_WithCorrectI2CBus)
{
	twim_read_packet_fake.custom_fake = twim_read_packet_custom_fake;
	ADCInitialise(buffer, bufferSize);
	FetchDiagnosticsData();
	EXPECT_EQ(twimI2CBuffer[0], &AVR32_TWIM0);
	EXPECT_EQ(twimI2CBuffer[1], &AVR32_TWIM0);
	EXPECT_EQ(twimI2CBuffer[2], &AVR32_TWIM0);
}


TEST_F(DiagnosticsADCs, TestThatFetchDiagnosticsUpdatesAllOuptutDataFields)
{
	twim_read_packet_fake.custom_fake = twim_read_packet_custom_fake_return_data;
	ADCInitialise(buffer, bufferSize);
	FetchDiagnosticsData();

	EXPECT_EQ(buffer[0], 0);
	EXPECT_EQ(buffer[1], 1);
	EXPECT_EQ(buffer[2], 2);
	EXPECT_EQ(buffer[3], 3);
	EXPECT_EQ(buffer[4], 0);
	EXPECT_EQ(buffer[5], 1);
	EXPECT_EQ(buffer[6], 2);
	EXPECT_EQ(buffer[7], 4);
	EXPECT_EQ(buffer[8], 5);
	EXPECT_EQ(buffer[9], 6);
	EXPECT_EQ(buffer[10], 7);
	EXPECT_EQ(buffer[11], 0);
	EXPECT_EQ(buffer[12], 1);
	EXPECT_EQ(buffer[13], 2);
	EXPECT_EQ(buffer[14], 3);
	EXPECT_EQ(buffer[15], 4);
	EXPECT_EQ(buffer[16], 5);
	EXPECT_EQ(buffer[17], 6);
	EXPECT_EQ(buffer[18], 7);
	EXPECT_EQ(buffer[19], 8);
	EXPECT_EQ(buffer[20], 9);
	EXPECT_EQ(buffer[21], 10);
}

TEST_F(DiagnosticsADCs, TestThatFetchDiagnosticsConvertsFrom16BitADCReadingTo10BitValue)
{
	twim_read_packet_fake.custom_fake = twim_read_packet_custom_fake_return_fixed_data;
	ADCInitialise(buffer, bufferSize);
	FetchDiagnosticsData();

	EXPECT_EQ(buffer[0], 0x3F0);
	EXPECT_EQ(buffer[21], 0x3F0);
}

/****************************************************/
/* Test reading of packets.							*/
/****************************************************/
status_code_t twim_read_packet_custom_fake(volatile avr32_twim_t * z_twim, const twim_package_t * z_twimPacket)
{
	twimI2CBuffer[twimBufferCount] = z_twim;
	twimPackageBuffer[twimBufferCount].addr_length = z_twimPacket->addr_length;
	twimPackageBuffer[twimBufferCount].buffer = z_twimPacket->buffer;
	twimPackageBuffer[twimBufferCount].chip = z_twimPacket->chip;
	twimPackageBuffer[twimBufferCount].length = z_twimPacket->length;
	twimPackageBuffer[twimBufferCount].no_wait = z_twimPacket->no_wait;
	twimBufferCount++;	
	return STATUS_OK;
}

status_code_t twim_read_packet_custom_fake_return_data(volatile avr32_twim_t * z_twim, const twim_package_t * z_twimPacket)
{
	for(uint32_t i = 0; i < (z_twimPacket->length/2); i++)
	{
		*(((uint16_t*)(z_twimPacket->buffer))+i) = i;
	}
	return  STATUS_OK;
}

status_code_t twim_read_packet_custom_fake_return_fixed_data(volatile avr32_twim_t * z_twim, const twim_package_t * z_twimPacket)
{
	for(uint32_t i = 0; i < (z_twimPacket->length/2); i++)
	{
		*(((uint16_t*)(z_twimPacket->buffer))+i) = 0xFFF0;
	}
	return  STATUS_OK;
}