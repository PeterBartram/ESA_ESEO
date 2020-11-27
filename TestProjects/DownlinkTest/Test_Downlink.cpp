#include "gtest/gtest.h"

extern "C"
{
	#include "fff.h"
	#include "Fake_CANODInterface.h"
	#include "Fake_FECencoder.h"
	#include "Fake_DACout.h"
	#include "Downlink.h"
	#include "Fake_CANObjectDictionary.h"
	#include <string.h>
	#include "TelemetryPacket.h"
	#include "Fake_SatelliteOperations.h"
	#include "Fake_PayloadData.h"
}

#define DOWNLINK_1K2 downlinkMode = (DATA_MODE)0x01; ChangeDownlinkMode(MODE_1K2)
#define DOWNLINK_4K8 downlinkMode = (DATA_MODE)0x02; ChangeDownlinkMode(MODE_4K8)
#define DOWNLINK_4K8_PAYLOAD downlinkMode = (DATA_MODE)0x03; ChangeDownlinkMode(MODE_4K8_PAYLOAD)
#define FITTER_MESSAGE "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"

uint32_t GetCANObjectDataRawReturnBuffer[10];
uint32_t GetCANObjectDataRawCount;

uint8_t FECedData[320];
DATA_MODE downlinkMode = MODE_RX_ONLY;

DEFINE_FFF_GLOBALS;

static OBJECT_DICTIONARY_ERROR GetCANObjectDataRaw(uint32_t z_index, uint32_t * z_data);
static uint8_t * EncodeFEC_FAKE( unsigned char *dataPtr, uint16_t * z_count);
static uint8_t * EncodeFEC_TestPacketRTT1( unsigned char *dataPtr, uint16_t * z_count);
static uint8_t * EncodeFEC_TestPacketRTT2( unsigned char *dataPtr, uint16_t * z_count);
static uint8_t * EncodeFEC_TestFrameNumber( unsigned char *dataPtr, uint16_t * z_count);
static uint8_t * EncodeFEC_TestPacketScieneData( unsigned char *dataPtr, uint16_t * z_count);
static uint8_t * EncodeFEC_TestPacketNoScienceDataPresent( unsigned char *dataPtr, uint16_t * z_count);
static PAYLOAD_DATA_ERROR GetPayloadDataFAKE(uint8_t * z_data, uint16_t z_size, uint16_t * z_packetNumber);


uint8_t frameNumberHistory[400];
uint8_t rttNumberHistory[400];
uint8_t historyCounter;
uint8_t firstPacket;

uint8_t payloadDataBuffer[254];
uint16_t payloadDataSize;
uint16_t packetNumber;
PAYLOAD_DATA_ERROR payloadRetVal = NO_ERROR_PAYLOAD_DATA;

class DownlinkTest : public testing::Test
{
public:

	void SetUp()
	{
		FFF_RESET_HISTORY();
		RESET_FAKE(InitFEC);
		RESET_FAKE(EncodeFEC);
		RESET_FAKE(InitialiseDAC);
		RESET_FAKE(DACoutBuffSize);
		RESET_FAKE(DACOutputBufferReady);
		RESET_FAKE(TransmitReady);
		RESET_FAKE(ChangeMode);
		RESET_FAKE(GetCANObjectDataRaw);
		RESET_FAKE(SetCANObjectDataRaw);
		RESET_FAKE(GetFitterMessage);
		RESET_FAKE(GetWODData);
		RESET_FAKE(GetPayloadData);

		memset(frameNumberHistory, 0, sizeof(frameNumberHistory));
		memset(rttNumberHistory, 0, sizeof(rttNumberHistory));
		historyCounter = 0;
		InitialiseDownlink();
		TransmitReady_fake.return_val = 1;
		memset(GetCANObjectDataRawReturnBuffer, 0, sizeof(GetCANObjectDataRawReturnBuffer));
		memset(FECedData, 0, sizeof(FECedData));
		downlinkMode = (DATA_MODE)0x01; //1k2
		GetCANObjectDataRawCount = 0;
		GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRaw;
		EncodeFEC_fake.return_val = (uint8_t*)1;
		firstPacket = 0;
		payloadRetVal = NO_ERROR_PAYLOAD_DATA;
	}

	virtual void TearDown() 
	{

	}
};

static PAYLOAD_DATA_ERROR GetPayloadDataFAKE(uint8_t * z_data, uint16_t z_size, uint16_t * z_packetNumber)
{
	if(z_data != 0 && z_packetNumber != 0)
	{
		for(uint32_t i = 0;i < z_size; i++)
		{
			z_data[i] = payloadDataBuffer[i];
		}
		*z_packetNumber = packetNumber;
	}
	return payloadRetVal;
}
static void GetFitterMessage_FAKE(uint8_t z_slot, uint8_t * z_buffer, uint16_t z_size)
{
	memcpy(z_buffer, FITTER_MESSAGE, z_size);
	z_buffer[0] = z_slot;
}

static OBJECT_DICTIONARY_ERROR GetCANObjectDataRawMirror(uint32_t z_index, uint32_t * z_data)
{
	if(z_data)
	{
		if(z_index == 0x24)	// Sequence Number
		{
			*z_data = 0x00242424;
		}
		else if(z_index == AMS_OBC_RFMODE)
		{
			*z_data = 2;
		}
		else if(z_index == AMS_OBC_DATAMODE)
		{
			*z_data = 1;
		}
		//else if(z_index == Payload transfer status)
		//{

		//}
		//else if(z_index == In eclipse mode)
		//{

		//}
		else if(z_index == 0x1F)	// Auto eclipse mode
		{
			*z_data = 1;
		}
		//else if(z_index == In safe mode)	
		//{

		//}
		else
		{
			*z_data = z_index;
		}
	}
	return NO_ERROR_OBJECT_DICTIONARY;
}

static OBJECT_DICTIONARY_ERROR GetCANObjectDataRaw(uint32_t z_index, uint32_t * z_data)
{
	*z_data = GetCANObjectDataRawReturnBuffer[GetCANObjectDataRawCount];
	GetCANObjectDataRawCount++;
	return NO_ERROR_OBJECT_DICTIONARY;
}

static uint8_t * EncodeFEC_FAKE( unsigned char *dataPtr, uint16_t * z_count)
{
	*z_count = sizeof(FECedData);
	return FECedData;
}

static uint8_t * EncodeFEC_TestFrameNumber( unsigned char *dataPtr, uint16_t * z_count)
{
	frameNumberHistory[historyCounter] = dataPtr[0] & 0x1F; //((dataPtr[0] & 0xF8) >> 3);
	historyCounter++;
	return FECedData;
}

static uint8_t * EncodeFEC_TestPacketFM1( unsigned char *dataPtr, uint16_t * z_count)
{
	uint8_t i = 0;
	uint8_t expectedOutput[254] = FITTER_MESSAGE;

	for(uint16_t i = 1; i < 200; i++)
	{
		EXPECT_EQ(expectedOutput[i], dataPtr[i+54]);
	}

	*z_count = sizeof(FECedData);
	return FECedData;
}

static uint8_t * EncodeFEC_TestPacketRTT1( unsigned char *dataPtr, uint16_t * z_count)
{
	uint8_t i = 0;
	uint8_t expectedOutput[0x33] = {0xc1, 0x04, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x43, 0x44, 0x45, 0x46, 0x00, 0x00, 0x23, 0x24, 0xea, 0x01, 0xc0, 0x00, 0xc2, 0x00, 0xc4, 0x8e, 0x00, 0x00, 0x00, 0x90, 0x92, 0x00, 0x00, 0x00, 0x96, 0x00, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00};

	for(uint16_t i = 1; i < 51; i++)
	{
		EXPECT_EQ(expectedOutput[i], dataPtr[i]);
	}

	*z_count = sizeof(FECedData);
	return FECedData;
}

static uint8_t * EncodeFEC_TestPacketRTT1IgnoreFirstPacket( unsigned char *dataPtr, uint16_t * z_count)
{
	uint8_t i = 0;
	uint8_t expectedOutput[0x33] = {19,4 ,64,65,66,67,68,69,70,71,72,73,74,80,81,82,83,84,85,96,97,98,99,36,36,36,37,234,1 ,80,1 ,82,1 ,84,1 ,1 ,0 ,0 ,2 ,5 ,1 ,0 ,0 ,32,1 ,0 ,0 ,34,1 ,0 ,0};

	if(firstPacket != 0)
	{
		for(uint16_t i = 1; i < 51; i++)
		{
			EXPECT_EQ(expectedOutput[i], dataPtr[i]);
		}
	}
	else
	{
		firstPacket++;
	}

	*z_count = sizeof(FECedData);
	return FECedData;
}

static uint8_t * EncodeFEC_TestPacketScieneData( unsigned char *dataPtr, uint16_t * z_count)
{
	uint8_t i = 0;
	uint8_t expectedOutput[256] = {0xdf, 0x04, 0x00, 0x05, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb};

	for(uint16_t i = 0; i < 256; i++)
	{
		if(expectedOutput[i] != dataPtr[i])
		{
			volatile uint32_t j = 4;
		}
		EXPECT_EQ(expectedOutput[i], dataPtr[i]);
	}

	*z_count = sizeof(FECedData);
	return FECedData;
}

static uint8_t * EncodeFEC_TestPacketNoScienceDataPresent( unsigned char *dataPtr, uint16_t * z_count)
{
	uint8_t i = 0;
	uint8_t expectedOutput[4] = {0xdf, 0x04, 0xFF, 0xFF};

	EXPECT_EQ(expectedOutput[0], dataPtr[0]);
	EXPECT_EQ(expectedOutput[1], dataPtr[1]);
	EXPECT_EQ(expectedOutput[2], dataPtr[2]);
	EXPECT_EQ(expectedOutput[3], dataPtr[3]);

	for(uint16_t i = 4; i < 256; i++)
	{
		EXPECT_EQ(0, dataPtr[i]);
	}

	*z_count = sizeof(FECedData);
	return FECedData;
}

static uint8_t * EncodeFEC_TestPacketRTT1IgnoreFirstTwoPackets( unsigned char *dataPtr, uint16_t * z_count)
{
	uint8_t i = 0;
	uint8_t expectedOutput[0x33] = {19,4 ,64,65,66,67,68,69,70,71,72,73,74,80,81,82,83,84,85,96,97,98,99,36,36,36,37,234,1 ,80,1 ,82,1 ,84,1 ,1 ,0 ,0 ,2 ,5 ,1 ,0 ,0 ,32,1 ,0 ,0 ,34,1 ,0 ,0};

	if(firstPacket > 1)
	{
		for(uint16_t i = 1; i < 51; i++)
		{
			EXPECT_EQ(expectedOutput[i], dataPtr[i]);
		}
	}
	else
	{
		firstPacket++;
	}

	*z_count = sizeof(FECedData);
	return FECedData;
}

static uint8_t * EncodeFEC_TestPacketRTT2( unsigned char *dataPtr, uint16_t * z_count)
{
	uint8_t i = 0;
	uint8_t expectedOutput[52] = {0xc2, 0x04, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x43, 0x44, 0x45, 0x46, 0x00, 0x00, 0x23, 0x24, 0xea, 0x01, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x00, 0xa0, 0x94, 0x00, 0xbc, 0xa8, 0x00, 0xac, 0x00, 0xae, 0x00, 0xb0, 0x00, 0xba};

	EXPECT_EQ((expectedOutput[0] & 0xC0), (dataPtr[0] & 0xC0));

	for(uint16_t i = 1; i < 52; i++)
	{
		EXPECT_EQ(expectedOutput[i], dataPtr[i]);
	}

	*z_count = sizeof(FECedData);
	return FECedData;
}

/****************************************************/
/* All modes										*/
/****************************************************/
TEST_F(DownlinkTest, InitialiseDownlinkCallsInitFEC)
{
	EXPECT_EQ(1, InitFEC_fake.call_count);
}

TEST_F(DownlinkTest, InitialiseDownlinkCallsInitDAC)
{
	EXPECT_EQ(1, InitialiseDAC_fake.call_count);
}

TEST_F(DownlinkTest, DownlinkTaskNothingHappensIfWeHaveNotBeenSignalledByTheDACInterrupt)
{
	TransmitReady_fake.return_val = 0;
	DownlinkTask();
	EXPECT_EQ(0, DACOutputBufferReady_fake.call_count);
	EXPECT_EQ(0, EncodeFEC_fake.call_count);
}

/****************************************************/
/* 1k2 Mode											*/
/****************************************************/
TEST_F(DownlinkTest, DownlinkTaskCallsModulatorSendIn1k2Mode)
{
	DOWNLINK_1K2;
	DownlinkTask();
	EXPECT_EQ(1, DACOutputBufferReady_fake.call_count);
}

TEST_F(DownlinkTest, DownlinkTaskCallsModulatorWithNonNullPointer)
{
	DOWNLINK_1K2;
	DownlinkTask();
	EXPECT_NE(0, (int)DACOutputBufferReady_fake.arg0_val);
}

TEST_F(DownlinkTest, DownlinkTaskCallsFECEncodeIn1k2Mode)
{
	DOWNLINK_1K2;
	DownlinkTask();
	EXPECT_EQ(1, EncodeFEC_fake.call_count);
}

TEST_F(DownlinkTest, DownlinkTaskCallsFECEncodeWithNonNullPointer)
{
	DOWNLINK_1K2;
	DownlinkTask();
	EXPECT_NE(0, (int)EncodeFEC_fake.arg0_val);
}

TEST_F(DownlinkTest, DownlinkTaskCallsFECencodeWithNonNUllCountPointer)
{
	DOWNLINK_1K2;
	DownlinkTask();
	EXPECT_NE(0, (int)EncodeFEC_fake.arg1_val);
}

TEST_F(DownlinkTest, DownlinkTaskCallsModulatorWithCorrectFECedSize)
{
	DOWNLINK_1K2;
	EncodeFEC_fake.custom_fake = EncodeFEC_FAKE;
	DownlinkTask();

	EXPECT_EQ(320, DACOutputBufferReady_fake.arg1_val);
}

TEST_F(DownlinkTest, DownlinkTaskCallsFECWithCorrectTelemetryData)
{
	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRawMirror;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT1;
	DOWNLINK_1K2;
	DownlinkTask();
}

TEST_F(DownlinkTest, DownlinkTaskCallsFECWithCorrectTelemetryDataForPacket2)
{
	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRawMirror;
	DOWNLINK_1K2;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT2;
	DownlinkTask();
}

TEST_F(DownlinkTest, TestWeAlternateBetweenRTT1andRTT2)
{
	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRawMirror;
	DOWNLINK_1K2;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT1;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT2;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT1;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT2;
	DownlinkTask();
}

TEST_F(DownlinkTest, TestWeUseCorrectFrameNumber1k2)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_1K2;

	for(uint32_t i = 0; i < 250; i++)
	{
		DownlinkTask();
	}

	for(uint32_t j = 0; j < 10; j++)
	{
		for(uint32_t i = 1; i < 25; i++)
		{
			EXPECT_EQ(i, frameNumberHistory[j*24+i-1]);
		}
	}
}

TEST_F(DownlinkTest, TestWeGetFitterMessageFourTimesForAll24FrameTypes)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_1K2;
	for(uint32_t i = 0; i < 24; i++){DownlinkTask();}
	EXPECT_EQ(GetFitterMessage_fake.call_count, 4);
}

TEST_F(DownlinkTest, TestWePassTheCorrectValueIntoFitterMessageGet)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_1K2;
	for(uint32_t i = 0; i < 24; i++){DownlinkTask();}

	for(uint32_t j = 0; j < 4; j++)
	{
		EXPECT_EQ(j, GetFitterMessage_fake.arg0_history[j]);
	}
}

TEST_F(DownlinkTest, TestWeDontPassANullPointerIntoFitterMessageGet)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_1K2;
	for(uint32_t i = 0; i < 24; i++){DownlinkTask();}

	for(uint32_t j = 0; j < 4; j++)
	{
		EXPECT_NE((void*)0, GetFitterMessage_fake.arg1_history[j]);
	}
	EXPECT_EQ(GetFitterMessage_fake.call_count, 4);	// No idea why I need this here for the test to pass - worrying and need to think about.
}

TEST_F(DownlinkTest, TestWePassInCorrectSizeToFitterMessageGet)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_1K2;
	for(uint32_t i = 0; i < 24; i++){DownlinkTask();}

	for(uint32_t j = 0; j < 4; j++)
	{
		EXPECT_EQ(200, GetFitterMessage_fake.arg2_history[j]);
	}
}

TEST_F(DownlinkTest, TestWeAddFitterMessageDataToBufferForAllFitterMessages)
{
	GetFitterMessage_fake.custom_fake = GetFitterMessage_FAKE;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	GetCANObjectDataRaw_fake.custom_fake = 0;
	DOWNLINK_1K2;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	DownlinkTask();
}


TEST_F(DownlinkTest, DownlinkTaskPassesAcrossCorrectData)
{
	DOWNLINK_1K2;
	EncodeFEC_fake.custom_fake = EncodeFEC_FAKE;

	for(uint32_t i = 0; i < sizeof(FECedData); i++){FECedData[i] = i;}
	DownlinkTask();
	for(uint32_t i = 0; i < sizeof(FECedData); i++)
	{
		if(i < 256)	EXPECT_EQ(i, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]);
		else EXPECT_EQ(i-256, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]);
	}	
}

TEST_F(DownlinkTest, DownlinkTaskCallsTelemetryGetTheRightNumberOfTimes)
{
	DOWNLINK_1K2;
	DownlinkTask();
	EXPECT_EQ(35, GetCANObjectDataRaw_fake.call_count);
}

/****************************************************/
/* 4k8 Mode											*/
/****************************************************/
TEST_F(DownlinkTest, 4k8_ChangingInto4k8ModeCausesPacketToBeFECedReadyForNextCycle)
{
	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRawMirror;
	DOWNLINK_1K2;
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DOWNLINK_4K8;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT1IgnoreFirstTwoPackets;
	DownlinkTask();
}

TEST_F(DownlinkTest, 4k8_ChangingInto4k8ModeCausesFECencodeToBeCalled)
{
	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRawMirror;
	DOWNLINK_1K2;
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	RESET_FAKE(EncodeFEC);
	DOWNLINK_4K8;
	DownlinkTask();
	EXPECT_EQ(2, EncodeFEC_fake.call_count);
}

TEST_F(DownlinkTest, DownlinkTaskCallsModulatorSendIn4k8Mode)
{
	DOWNLINK_4K8;
	DownlinkTask();
	EXPECT_EQ(1, DACOutputBufferReady_fake.call_count);
}

TEST_F(DownlinkTest, DownlinkTaskCallsModulatorWithNonNullPointer4k8)
{
	DOWNLINK_4K8;
	DownlinkTask();
	EXPECT_NE(0, (int)DACOutputBufferReady_fake.arg0_val);
}

TEST_F(DownlinkTest, DownlinkTaskCallsModulatorWithCorrectFECedSize4k8)
{
	DOWNLINK_4K8;
	EncodeFEC_fake.custom_fake = EncodeFEC_FAKE;
	DownlinkTask();

	EXPECT_EQ(650, DACOutputBufferReady_fake.arg1_val);
}

TEST_F(DownlinkTest, DownlinkTaskCallsFECEncodeIn4k8Mode)
{
	DOWNLINK_4K8;
	EXPECT_EQ(1, EncodeFEC_fake.call_count);
}

TEST_F(DownlinkTest, DownlinkTaskCallsFECEncodeWithNonNullPointer4k8)
{
	DOWNLINK_4K8;
	DownlinkTask();
	EXPECT_NE(0, (int)EncodeFEC_fake.arg0_val);
}

TEST_F(DownlinkTest, DownlinkTaskCallsFECencodeWithNonNUllCountPointer4k8)
{
	DOWNLINK_4K8;
	DownlinkTask();
	EXPECT_NE(0, (int)EncodeFEC_fake.arg1_val);
}

TEST_F(DownlinkTest, DownlinkTaskPassesAcrossCorrectData4k8DelayedByOneTurn)
{
	DOWNLINK_4K8;
	EncodeFEC_fake.custom_fake = EncodeFEC_FAKE;

	for(uint32_t i = 0; i < sizeof(FECedData); i++){FECedData[i] = i;}
	DownlinkTask();
	DownlinkTask();

	for(uint32_t i = 2; i < sizeof(FECedData); i++)	// 2 because we need to ignore the first two bytes sent across
	{
		if(i < 256)	EXPECT_EQ(i, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]);
		else EXPECT_EQ(i-256, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]);
	}
}

TEST_F(DownlinkTest, DownlinkTaskPassesAcrossCorrectData4k8DelayedByOneTurnRepeatedly)
{
	DOWNLINK_1K2;
	EncodeFEC_fake.custom_fake = EncodeFEC_FAKE;
	DownlinkTask();	
	
	for(uint32_t i = 0; i < sizeof(FECedData); i++){FECedData[i] = 0xAA;}
	GetCANObjectDataRawCount = 0;
	DOWNLINK_4K8;
	for(uint32_t i = 0; i < sizeof(FECedData); i++){FECedData[i] = 0xBB;}
	GetCANObjectDataRawCount = 0;
	DownlinkTask();
	for(uint32_t i = 2; i < sizeof(FECedData); i++){EXPECT_EQ(0xAA, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]); } // 2 because we need to ignore the first two bytes sent across
	GetCANObjectDataRawCount = 0;
	DownlinkTask();
	for(uint32_t i = 2; i < sizeof(FECedData); i++){EXPECT_EQ(0xBB, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]); }	// 2 because we need to ignore the first two bytes sent across

	GetCANObjectDataRawCount = 0;
	DownlinkTask();
	for(uint32_t i = 0; i < sizeof(FECedData); i++){FECedData[i] = 0xAA;}
	GetCANObjectDataRawCount = 0;
	DownlinkTask();
	for(uint32_t i = 0; i < sizeof(FECedData); i++){FECedData[i] = 0xBB;}
	GetCANObjectDataRawCount = 0;
	DownlinkTask();
	for(uint32_t i = 2; i < sizeof(FECedData); i++){EXPECT_EQ(0xAA, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]); }
	GetCANObjectDataRawCount = 0;
	DownlinkTask();
	for(uint32_t i = 2; i < sizeof(FECedData); i++){EXPECT_EQ(0xBB, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]); }
}

TEST_F(DownlinkTest, 4k8_DownlinkTaskCallsFECWithCorrectTelemetryDataForPacket2)
{
	DOWNLINK_1K2;
	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRawMirror;
	DOWNLINK_4K8;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT2;
}

TEST_F(DownlinkTest, 4k8_TestWeAlternateBetweenRTT1andRTT2)
{
	DOWNLINK_1K2;
	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRawMirror;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT1;
	DOWNLINK_4K8;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT2;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT1;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT2;
	DownlinkTask();
}


TEST_F(DownlinkTest, 4k8_TestWeUseCorrectFrameNumber)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_4K8;

	for(uint32_t i = 0; i < 250; i++)
	{
		DownlinkTask();
	}

	for(uint32_t j = 0; j < 10; j++)
	{
		for(uint32_t i = 1; i < 25; i++)
		{
			EXPECT_EQ(i, frameNumberHistory[j*24+i-1]);
		}
	}
}

TEST_F(DownlinkTest, 4k8_TestWeGetFitterMessageFourTimesForAll24FrameTypes)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_4K8;
	for(uint32_t i = 0; i < 23; i++){DownlinkTask();}
	EXPECT_EQ(GetFitterMessage_fake.call_count, 4);
}

TEST_F(DownlinkTest, 4k8_TestWePassTheCorrectValueIntoFitterMessageGet)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_4K8;
	for(uint32_t i = 0; i < 24; i++){DownlinkTask();}

	for(uint32_t j = 0; j < 4; j++)
	{
		EXPECT_EQ(j, GetFitterMessage_fake.arg0_history[j]);
	}
}

TEST_F(DownlinkTest, 4k8_TestWeDontPassANullPointerIntoFitterMessageGet)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_4K8;
	for(uint32_t i = 0; i < 23; i++){DownlinkTask();}

	for(uint32_t j = 0; j < 4; j++)
	{
		EXPECT_NE((void*)0, GetFitterMessage_fake.arg1_history[j]);
	}
}

TEST_F(DownlinkTest, 4k8_TestWePassInCorrectSizeToFitterMessageGet)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_4K8;
	for(uint32_t i = 0; i < 24; i++){DownlinkTask();}

	for(uint32_t j = 0; j < 4; j++)
	{
		EXPECT_EQ(200, GetFitterMessage_fake.arg2_history[j]);
	}
}

TEST_F(DownlinkTest, 4k8_TestWeAddFitterMessageDataToBufferForAllFitterMessages)
{
	GetFitterMessage_fake.custom_fake = GetFitterMessage_FAKE;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	GetCANObjectDataRaw_fake.custom_fake = 0;
	DOWNLINK_4K8;
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	DownlinkTask();
}



/****************************************************/
/* 4k8 Payload Mode											*/
/****************************************************/
TEST_F(DownlinkTest, 4k8Payload_ChangingInto4k8ModeCausesPacketToBeFECedReadyForNextCycle)
{
	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRawMirror;
	DOWNLINK_1K2;
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DOWNLINK_4K8_PAYLOAD;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT1IgnoreFirstTwoPackets;
	DownlinkTask();
}

TEST_F(DownlinkTest, 4k8Payload_ChangingInto4k8ModeCausesFECencodeToBeCalled)
{
	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRawMirror;
	DOWNLINK_1K2;
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	RESET_FAKE(EncodeFEC);
	DOWNLINK_4K8_PAYLOAD;
	DownlinkTask();
	EXPECT_EQ(2, EncodeFEC_fake.call_count);
}

TEST_F(DownlinkTest, DownlinkTaskCallsModulatorSendIn4k8PayloadMode)
{
	DOWNLINK_4K8_PAYLOAD;
	DownlinkTask();
	EXPECT_EQ(1, DACOutputBufferReady_fake.call_count);
}

TEST_F(DownlinkTest, DownlinkTaskCallsModulatorWithNonNullPointer4k8Payload)
{
	DOWNLINK_4K8_PAYLOAD;
	DownlinkTask();
	EXPECT_NE(0, (int)DACOutputBufferReady_fake.arg0_val);
}

TEST_F(DownlinkTest, DownlinkTaskCallsModulatorWithCorrectFECedSize4k8Payload)
{
	DOWNLINK_4K8_PAYLOAD;
	EncodeFEC_fake.custom_fake = EncodeFEC_FAKE;
	DownlinkTask();

	EXPECT_EQ(320, DACOutputBufferReady_fake.arg1_val);
}

TEST_F(DownlinkTest, DownlinkTaskCallsFECEncodeIn4k8PayloadMode)
{
	DOWNLINK_4K8_PAYLOAD;
	EXPECT_EQ(1, EncodeFEC_fake.call_count);
}

TEST_F(DownlinkTest, DownlinkTaskCallsFECEncodeWithNonNullPointer4k8Payload)
{
	DOWNLINK_4K8_PAYLOAD;
	DownlinkTask();
	EXPECT_NE(0, (int)EncodeFEC_fake.arg0_val);
}

TEST_F(DownlinkTest, DownlinkTaskCallsFECencodeWithNonNUllCountPointer4k8Payload)
{
	DOWNLINK_4K8_PAYLOAD;
	DownlinkTask();
	EXPECT_NE(0, (int)EncodeFEC_fake.arg1_val);
}

TEST_F(DownlinkTest, DownlinkTaskPassesAcrossCorrectData4k8PayloadDelayedByOneTurn)
{
	DOWNLINK_4K8_PAYLOAD;
	EncodeFEC_fake.custom_fake = EncodeFEC_FAKE;

	for(uint32_t i = 0; i < sizeof(FECedData); i++){FECedData[i] = i;}
	DownlinkTask();
	DownlinkTask();

	for(uint32_t i = 2; i < sizeof(FECedData); i++)	// 2 because we need to ignore the first two bytes sent across
	{
		if(i < 256)	EXPECT_EQ(i, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]);
		else EXPECT_EQ(i-256, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]);
	}
}

TEST_F(DownlinkTest, DownlinkTaskPassesAcrossCorrectData4k8PayloadDelayedByOneTurnRepeatedly)
{
	DOWNLINK_1K2;
	EncodeFEC_fake.custom_fake = EncodeFEC_FAKE;
	DownlinkTask();	
	
	for(uint32_t i = 0; i < sizeof(FECedData); i++){FECedData[i] = 0xAA;}
	GetCANObjectDataRawCount = 0;
	DOWNLINK_4K8_PAYLOAD;
	for(uint32_t i = 0; i < sizeof(FECedData); i++){FECedData[i] = 0xBB;}
	GetCANObjectDataRawCount = 0;
	DownlinkTask();
	for(uint32_t i = 2; i < sizeof(FECedData); i++){EXPECT_EQ(0xAA, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]); } // 2 because we need to ignore the first two bytes sent across
	GetCANObjectDataRawCount = 0;
	DownlinkTask();
	for(uint32_t i = 2; i < sizeof(FECedData); i++){EXPECT_EQ(0xBB, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]); }	// 2 because we need to ignore the first two bytes sent across

	GetCANObjectDataRawCount = 0;
	DownlinkTask();
	for(uint32_t i = 0; i < sizeof(FECedData); i++){FECedData[i] = 0xAA;}
	GetCANObjectDataRawCount = 0;
	DownlinkTask();
	for(uint32_t i = 0; i < sizeof(FECedData); i++){FECedData[i] = 0xBB;}
	GetCANObjectDataRawCount = 0;
	DownlinkTask();
	for(uint32_t i = 2; i < sizeof(FECedData); i++){EXPECT_EQ(0xAA, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]); }
	GetCANObjectDataRawCount = 0;
	DownlinkTask();
	for(uint32_t i = 2; i < sizeof(FECedData); i++){EXPECT_EQ(0xBB, ((uint8_t*)DACOutputBufferReady_fake.arg0_val)[i]); }
}

TEST_F(DownlinkTest, 4k8Payload_DownlinkTaskCallsFECWithCorrectTelemetryDataForPacket2)
{
	DOWNLINK_1K2;
	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRawMirror;
	DOWNLINK_4K8_PAYLOAD;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT2;
}

TEST_F(DownlinkTest, 4k8Payload_TestWeAlternateBetweenRTT1andRTT2)
{
	DOWNLINK_1K2;
	GetCANObjectDataRaw_fake.custom_fake = GetCANObjectDataRawMirror;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT1;
	DOWNLINK_4K8_PAYLOAD;
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT2;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT1;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketRTT2;
	DownlinkTask();
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
}


TEST_F(DownlinkTest, 4k8Payload_TestWeUseCorrectFrameNumber)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_4K8_PAYLOAD;

	for(uint32_t i = 0; i < 250; i++)
	{
		DownlinkTask();
	}

	EXPECT_EQ(1, frameNumberHistory[0]);
	EXPECT_EQ(2, frameNumberHistory[4]);
	EXPECT_EQ(3, frameNumberHistory[8]);
	EXPECT_EQ(4, frameNumberHistory[12]);
	EXPECT_EQ(5, frameNumberHistory[16]);
	EXPECT_EQ(6, frameNumberHistory[20]);
	EXPECT_EQ(7, frameNumberHistory[24]);
	EXPECT_EQ(8, frameNumberHistory[28]);
	EXPECT_EQ(9, frameNumberHistory[32]);
	EXPECT_EQ(10, frameNumberHistory[36]);
	EXPECT_EQ(11, frameNumberHistory[40]);
	EXPECT_EQ(12, frameNumberHistory[44]);
	EXPECT_EQ(13, frameNumberHistory[48]);
	EXPECT_EQ(14, frameNumberHistory[52]);
	EXPECT_EQ(15, frameNumberHistory[56]);
	EXPECT_EQ(16, frameNumberHistory[60]);
	EXPECT_EQ(17, frameNumberHistory[64]);
	EXPECT_EQ(18, frameNumberHistory[68]);
	EXPECT_EQ(19, frameNumberHistory[72]);
	EXPECT_EQ(20, frameNumberHistory[76]);
	EXPECT_EQ(21, frameNumberHistory[80]);
	EXPECT_EQ(22, frameNumberHistory[84]);
	EXPECT_EQ(23, frameNumberHistory[88]);
	EXPECT_EQ(24, frameNumberHistory[92]);
	EXPECT_EQ(1, frameNumberHistory[96]);	
}

TEST_F(DownlinkTest, 4k8Payload_TestWeGetFitterMessageFourTimesForAll24FrameTypes)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_4K8_PAYLOAD;
	for(uint32_t i = 0; i < 95; i++){DownlinkTask();}
	EXPECT_EQ(GetFitterMessage_fake.call_count, 4);
}

TEST_F(DownlinkTest, 4k8Payload_TestWePassTheCorrectValueIntoFitterMessageGet)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_4K8_PAYLOAD;
	for(uint32_t i = 0; i < 96; i++){DownlinkTask();}

	for(uint32_t j = 0; j < 4; j++)
	{
		EXPECT_EQ(j, GetFitterMessage_fake.arg0_history[j]);
	}
}

TEST_F(DownlinkTest, 4k8Payload_TestWeDontPassANullPointerIntoFitterMessageGet)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_4K8_PAYLOAD;
	for(uint32_t i = 0; i < 96; i++){DownlinkTask();}

	for(uint32_t j = 0; j < 4; j++)
	{
		EXPECT_NE((void*)0, GetFitterMessage_fake.arg1_history[j]);
	}
}

TEST_F(DownlinkTest, 4k8Payload_TestWePassInCorrectSizeToFitterMessageGet)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_4K8_PAYLOAD;
	for(uint32_t i = 0; i < 96; i++){DownlinkTask();}

	for(uint32_t j = 0; j < 4; j++)
	{
		EXPECT_EQ(200, GetFitterMessage_fake.arg2_history[j]);
	}
}

TEST_F(DownlinkTest, 4k8Payload_TestWeAddFitterMessageDataToBufferForAllFitterMessages)
{
	GetFitterMessage_fake.custom_fake = GetFitterMessage_FAKE;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	GetCANObjectDataRaw_fake.custom_fake = 0;
	DOWNLINK_4K8_PAYLOAD;
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	EXPECT_EQ(1, GetFitterMessage_fake.call_count);
	DownlinkTask();
	EXPECT_EQ(2, GetFitterMessage_fake.call_count);	
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	EXPECT_EQ(2, GetFitterMessage_fake.call_count);
	DownlinkTask();
	EXPECT_EQ(3, GetFitterMessage_fake.call_count);
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	EXPECT_EQ(3, GetFitterMessage_fake.call_count);
	DownlinkTask();
	EXPECT_EQ(4, GetFitterMessage_fake.call_count);
	EncodeFEC_fake.custom_fake = 0;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketFM1;
	EXPECT_EQ(4, GetFitterMessage_fake.call_count);
	DownlinkTask();
	EXPECT_EQ(5, GetFitterMessage_fake.call_count);
}

TEST_F(DownlinkTest, 1k2_TestWeAddCallGetWODRightNumberOfTimes)
{
	DOWNLINK_1K2;

	for(uint32_t i = 0; i < 24; i++)
	{
		DownlinkTask();
	}
	EXPECT_EQ(20, GetWODData_fake.call_count);
}

TEST_F(DownlinkTest, 1k2_TestWeAddPassCorrectFrameNumberToGetWOD)
{
	DOWNLINK_1K2;

	for(uint32_t i = 0; i < 24; i++)
	{
		DownlinkTask();
	}
	for(uint32_t i = 0; i < 20; i++)
	{
		EXPECT_EQ(i+1, GetWODData_fake.arg0_history[i]);
	}
}

TEST_F(DownlinkTest, 4k8Payload_CallsPayloadDataCorrectNumberOfTimes)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_4K8_PAYLOAD;
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	DownlinkTask();
	
	EXPECT_EQ(4, GetPayloadData_fake.call_count);
}

TEST_F(DownlinkTest, 4k8Payload_CallsGetPayloadDataWithCorrectParameters)
{
	EncodeFEC_fake.custom_fake = EncodeFEC_TestFrameNumber;
	DOWNLINK_4K8_PAYLOAD;
	DownlinkTask();
	
	EXPECT_NE((uint8_t*)0, GetPayloadData_fake.arg0_history[0]);
	EXPECT_EQ(252, GetPayloadData_fake.arg1_history[0]);
	EXPECT_NE((uint16_t*)0, GetPayloadData_fake.arg2_history[0]);
}

TEST_F(DownlinkTest, 4k8Payload_GetPayloadDataPassesCorrectDataIntoEncodeFEC)
{
	GetPayloadData_fake.custom_fake = GetPayloadDataFAKE;
	packetNumber = 5;
	DOWNLINK_4K8_PAYLOAD;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketScieneData;
	for(uint32_t i = 0; i < 252; i++)
	{
		payloadDataBuffer[i] = i;
	}
	DownlinkTask();
}

TEST_F(DownlinkTest, 4k8Payload_GetPayloadDataPassesCorrectDataIntoEncodeFECWhenNoPayloadDataPresent)
{
	GetPayloadData_fake.custom_fake = GetPayloadDataFAKE;
	payloadRetVal = NO_DATA_READY;

	packetNumber = 5;
	DOWNLINK_4K8_PAYLOAD;
	EncodeFEC_fake.custom_fake = EncodeFEC_TestPacketNoScienceDataPresent;
	for(uint32_t i = 0; i < 252; i++)
	{
		payloadDataBuffer[i] = i;
	}
	DownlinkTask();
}



