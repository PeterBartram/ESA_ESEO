#include "gtest/gtest.h"

extern "C"
{
	#include "Diagnostics.h"
	#include "ADCDriver.h"
	#include "Fake_ADCDriver.h"
	#include "Fake_queue.h"	
	#include "Fake_CANODInterface.h"
	#include <string.h>
	#include "fff.h"
}

/* Custom fakes. */
static ADC_ERROR ADCInitialiseCustomFake(uint16_t * z_buffer, uint32_t z_size);
static ADC_ERROR FetchDiagnosticsDataCustomFake(void);
uint16_t * p_diagnosticData;
uint32_t z_diagnosticDataSize;	

extern "C"
{
	/* Decelerations here for static functions within the ADCDriver itself. */
	//ADC_ERROR FetchDiagnosticsData(SENSOR_NAME z_name, uint32_t * z_buffer);
}

static uint32_t xQueueSendFAKE(QueueHandle_t z_queue, const void * z_data, uint32_t z_ticks);

uint8_t outputBuffer[7][22];
uint8_t outputCount;

DEFINE_FFF_GLOBALS;

class Diagnostics : public testing::Test
{
public:
	uint16_t diagnosticData[50];
	uint32_t diagnosticDataSize;

	void SetUp()
	{
		//ADCInitialise_fake.return_val = NO_ERROR_ADC;
		//diagnosticDataSize = sizeof(diagnosticData)/sizeof(diagnosticData[0]);
		memset(outputBuffer, 0, sizeof(outputBuffer));
		outputCount = 0;
	}

	virtual void TearDown() 
	{
		UninitialiseDiagnostics();
		RESET_FAKE(ADCInitialise);
		RESET_FAKE(ADCUninitialise);
		RESET_FAKE(FetchDiagnosticsData);
		//FetchReturnSequenceCount = 0;
	}
};


TEST_F(Diagnostics, InitialiseReturnErrorIfAdcDriverInitialisationFails)
{
	ADCInitialise_fake.return_val = COULD_NOT_INITIALISE_ADC;
	diagnosticDataSize = 50;
	EXPECT_EQ(InitialiseDiagnostics(diagnosticData, diagnosticDataSize), INITIALISATION_FAILED_DIAG);
}

TEST_F(Diagnostics, InitialiseReturnNoErrorIfAdcInitialisationSucceeds)
{
	ADCInitialise_fake.return_val = NO_ERROR_ADC;
	diagnosticDataSize = 50;
	EXPECT_EQ(InitialiseDiagnostics(diagnosticData, diagnosticDataSize), NO_ERROR_DIAG);
}

TEST_F(Diagnostics, TestUninitialiseDiagnoticsCallsUninitialiseADC)
{
	ADCInitialise_fake.return_val = NO_ERROR_ADC;
	InitialiseDiagnostics(diagnosticData, diagnosticDataSize);
	UninitialiseDiagnostics();
	EXPECT_EQ(ADCUninitialise_fake.call_count, 1);
}

TEST_F(Diagnostics, ADCInitialiseIsCalled)
{
	diagnosticDataSize = 50;
	InitialiseDiagnostics(diagnosticData, diagnosticDataSize);
	EXPECT_EQ(ADCInitialise_fake.call_count, 1);
}

TEST_F(Diagnostics, ADCInitialiseHasBufferPassedIntoItCorrectly)
{
	diagnosticDataSize = 50;
	InitialiseDiagnostics(diagnosticData, diagnosticDataSize);
	EXPECT_EQ(ADCInitialise_fake.arg0_val, diagnosticData);
}

TEST_F(Diagnostics, ADCInitialiseHasCorrectBufferSizePassedIn)
{
	diagnosticDataSize = 50;
	InitialiseDiagnostics(diagnosticData, diagnosticDataSize);
	EXPECT_EQ(ADCInitialise_fake.arg1_val, 50);
}

TEST_F(Diagnostics, InitialiseReturnErrorIfDiagnosticDataReturnIsNotProvided)
{
	ADCInitialise_fake.return_val = (ADC_ERROR)1;
	EXPECT_EQ(InitialiseDiagnostics(0,100), INITIALISATION_FAILED_DIAG);
}

TEST_F(Diagnostics, InitialiseReturnErrorIfDiagnosticDataReturnSizeIsZero)
{
	ADCInitialise_fake.return_val = (ADC_ERROR)1;
	EXPECT_EQ(InitialiseDiagnostics(diagnosticData, 0), INITIALISATION_FAILED_DIAG);
}

TEST_F(Diagnostics, InitialiseReturnNoErrorIfDiagnosticsCorrectlyInitialises)
{
	diagnosticDataSize = 50;
	EXPECT_EQ(InitialiseDiagnostics(diagnosticData,diagnosticDataSize), NO_ERROR_DIAG);
}

TEST_F(Diagnostics, FetchDiagnosticsReturnsErrorIfNotInitialised)
{
	EXPECT_EQ(FetchNewDiagnosticData(), NOT_INITIALISED_DIAG);
}

TEST_F(Diagnostics, FetchDiagnosticsReturnsNoErrorIfInitialised)
{
	InitialiseDiagnostics(diagnosticData, 10);
	EXPECT_EQ(FetchNewDiagnosticData(), NO_ERROR_DIAG);
}

TEST_F(Diagnostics, FetchDiagnosticsCallsFetchDiagnosticData)
{
	InitialiseDiagnostics(diagnosticData, 10);
	FetchNewDiagnosticData();
	EXPECT_EQ(FetchDiagnosticsData_fake.call_count, 1);
}

TEST_F(Diagnostics, FetchDiagnosticsReturnsNoErrorIfADCDriverReturnsNoError)
{
	InitialiseDiagnostics(diagnosticData, 10);
	FetchNewDiagnosticData();
	FetchDiagnosticsData_fake.return_val = NO_ERROR_ADC;
	EXPECT_EQ(FetchDiagnosticsData_fake.return_val, NO_ERROR_DIAG);
}

TEST_F(Diagnostics, FetchDiagnosticsReturnsErrorIfADCDriverReturnsError)
{
	InitialiseDiagnostics(diagnosticData, 10);
	FetchNewDiagnosticData();
	FetchDiagnosticsData_fake.return_val = COULD_NOT_READ_ADC;
	EXPECT_NE(FetchDiagnosticsData_fake.return_val, NO_ERROR_DIAG);
}

TEST_F(Diagnostics, FetchDiagnosticsReturnsCorrectData)
{
	FetchDiagnosticsData_fake.custom_fake = FetchDiagnosticsDataCustomFake;
	ADCInitialise_fake.custom_fake = ADCInitialiseCustomFake;
	InitialiseDiagnostics(diagnosticData, 50);
	FetchNewDiagnosticData();
	
	for(uint32_t i = 0; i < 50; i++)
	{
		EXPECT_EQ(diagnosticData[i], i);
	}
}

TEST_F(Diagnostics, FetchDiagnosticsAddsCorrectDataToQueue)
{
	FetchDiagnosticsData_fake.custom_fake = FetchDiagnosticsDataCustomFake;
	ADCInitialise_fake.custom_fake = ADCInitialiseCustomFake;
	xQueueSend_fake.custom_fake = xQueueSendFAKE;
	GetCommandQueueHandle_fake.return_val = 1;
	InitialiseDiagnostics(diagnosticData, 22);
	FetchNewDiagnosticData();

	EXPECT_EQ(outputBuffer[0][0], 0);
	EXPECT_EQ(outputBuffer[1][0], 5);
	EXPECT_EQ(outputBuffer[2][0], AMS_L_RSSI_TRANS);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][0], 0x00);
	

	EXPECT_EQ(outputBuffer[0][1], 0);
	EXPECT_EQ(outputBuffer[1][1], 5);
	EXPECT_EQ(outputBuffer[2][1], AMS_L_DOPPLER_COMMAND);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][1], 0x01 >> 2);
	
	EXPECT_EQ(outputBuffer[0][2], 0);
	EXPECT_EQ(outputBuffer[1][2], 5);
	EXPECT_EQ(outputBuffer[2][2], AMS_L_RSSI_COMMAND);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][2], 0x02 >> 2);
	
	EXPECT_EQ(outputBuffer[0][3], 0);
	EXPECT_EQ(outputBuffer[1][3], 5);
	EXPECT_EQ(outputBuffer[2][3], AMS_L_T);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][3], 0x03 >> 2);
	
	EXPECT_EQ(outputBuffer[0][4], 0);
	EXPECT_EQ(outputBuffer[1][4], 5);
	EXPECT_EQ(outputBuffer[2][4], AMS_VHF_BPSK_PA_T);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][4], 0x04 >> 2);
	
	EXPECT_EQ(outputBuffer[0][5], 0);
	EXPECT_EQ(outputBuffer[1][5], 5);
	EXPECT_EQ(outputBuffer[2][5], AMS_VHF_FP);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][5], 0x05 >> 2);
	
	EXPECT_EQ(outputBuffer[0][6], 0);
	EXPECT_EQ(outputBuffer[1][6], 5);
	EXPECT_EQ(outputBuffer[2][6], AMS_VHF_RP);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][6], 0x06 >> 2);
	
	EXPECT_EQ(outputBuffer[0][7], 0);
	EXPECT_EQ(outputBuffer[1][7], 5);
	EXPECT_EQ(outputBuffer[2][7], AMS_VHF_BPSK_PA_I);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][7], 0x07 >> 2);
	
	EXPECT_EQ(outputBuffer[0][8], 0);
	EXPECT_EQ(outputBuffer[1][8], 5);
	EXPECT_EQ(outputBuffer[2][8], AMS_VHF_FM_PA_I);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][8], 0x08 >> 2);
	
	EXPECT_EQ(outputBuffer[0][9], 0);
	EXPECT_EQ(outputBuffer[1][9], 5);
	EXPECT_EQ(outputBuffer[2][9], AMS_VHF_FM_PA_T);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][9], 0x09 >> 2);
	
	EXPECT_EQ(outputBuffer[0][10], 0);
	EXPECT_EQ(outputBuffer[1][10], 5);
	EXPECT_EQ(outputBuffer[2][10], AMS_VHF_BPSK_I);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][10], 0x0A >> 2);
	
	EXPECT_EQ(outputBuffer[0][11], 0);
	EXPECT_EQ(outputBuffer[1][11], 5);
	EXPECT_EQ(outputBuffer[2][11], AMS_EPS_CCT_T);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][11], 0x0B >> 2);
	
	EXPECT_EQ(outputBuffer[0][12], 0);
	EXPECT_EQ(outputBuffer[1][12], 5);
	EXPECT_EQ(outputBuffer[2][12], AMS_EPS_ENC_T);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][12], 0x0C >> 2);
	
	EXPECT_EQ(outputBuffer[0][13], 0);
	EXPECT_EQ(outputBuffer[1][13], 5);
	EXPECT_EQ(outputBuffer[2][13], AMS_EPS_DCDC_T);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][13], 0x0D >> 2);
	
	EXPECT_EQ(outputBuffer[0][14], 0);
	EXPECT_EQ(outputBuffer[1][14], 5);
	EXPECT_EQ(outputBuffer[2][14], AMS_EPS_DCDC_I);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][14], 0x0E >> 2);
	
	EXPECT_EQ(outputBuffer[0][15], 0);
	EXPECT_EQ(outputBuffer[1][15], 5);
	EXPECT_EQ(outputBuffer[2][15], AMS_EPS_DCDC_V);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][15], 0x0F >> 2);
	
	EXPECT_EQ(outputBuffer[0][16], 0);
	EXPECT_EQ(outputBuffer[1][16], 5);
	EXPECT_EQ(outputBuffer[2][16], AMS_EPS_6V_V);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][16], 0x10 >> 2);
	
	EXPECT_EQ(outputBuffer[0][17], 0);
	EXPECT_EQ(outputBuffer[1][17], 5);
	EXPECT_EQ(outputBuffer[2][17], AMS_EPS_9V_V);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][17], 0x11 >> 2);
	
	EXPECT_EQ(outputBuffer[0][18], 0);
	EXPECT_EQ(outputBuffer[1][18], 5);
	EXPECT_EQ(outputBuffer[2][18], AMS_EPS_3V3_V);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][18], 0x12 >> 2);
	
	EXPECT_EQ(outputBuffer[0][19], 0);
	EXPECT_EQ(outputBuffer[1][19], 5);
	EXPECT_EQ(outputBuffer[2][19], AMS_EPS_6V_I);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][19], 0x13 >> 2);
	
	EXPECT_EQ(outputBuffer[0][20], 0);
	EXPECT_EQ(outputBuffer[1][20], 5);
	EXPECT_EQ(outputBuffer[2][20], AMS_EPS_3V3_I);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][20], 0x14 >> 2);
	
	EXPECT_EQ(outputBuffer[0][21], 0);
	EXPECT_EQ(outputBuffer[1][21], 5);
	EXPECT_EQ(outputBuffer[2][21], AMS_EPS_9V_I);
	EXPECT_EQ(outputBuffer[3][0], 0x00);
	EXPECT_EQ(outputBuffer[4][0], 0x00);
	EXPECT_EQ(outputBuffer[5][0], 0x00);
	EXPECT_EQ(outputBuffer[6][21], 0x15 >> 2);
}

static uint32_t xQueueSendFAKE(QueueHandle_t z_queue, const void * z_data, uint32_t z_ticks)
{
	outputBuffer[0][outputCount] = ((uint8_t*)z_data)[0];
	outputBuffer[1][outputCount] = ((uint8_t*)z_data)[1];
	outputBuffer[2][outputCount] = ((uint8_t*)z_data)[2];
	outputBuffer[3][outputCount] = ((uint8_t*)z_data)[3];
	outputBuffer[4][outputCount] = ((uint8_t*)z_data)[4];
	outputBuffer[5][outputCount] = ((uint8_t*)z_data)[5];
	outputBuffer[6][outputCount] = ((uint8_t*)z_data)[6];
	outputCount++;
	return 1;
}

static ADC_ERROR FetchDiagnosticsDataCustomFake(void)
{
	for(uint32_t i = 0; i < 50; i++)
	{
		p_diagnosticData[i] = i;
	}	
	return NO_ERROR_ADC;
}

static ADC_ERROR ADCInitialiseCustomFake(uint16_t * z_buffer, uint32_t z_size)
{
	p_diagnosticData = z_buffer;
	z_diagnosticDataSize = z_size;
	return NO_ERROR_ADC;
}
