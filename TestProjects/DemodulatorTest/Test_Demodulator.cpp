#include "gtest/gtest.h"

extern "C"
{
	#include "fff.h"
	#include "Uplink.h"
	#include "BCH.h"
	#include <stdio.h>
	#include "queue.h"
	#include "Fake_queue.h"
	#include "Fake_CANODInterface.h"
}

DEFINE_FFF_GLOBALS;

#define PREAMBLE_LENGTH 100

#define ADD_PACKET_ONE_SMALL														\
	size += generateAFSKPrecise(0x00000000, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x1ACFFC1D, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x01040000, &preciseArray[size], 32, &phase);

#define ADD_PACKET_TWO_SMALL														\
	size += generateAFSKPrecise(0x00000000, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x1ACFFC1D, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x01AA0000, &preciseArray[size], 32, &phase);

#define TEST_PACKET_ONE_SMALL	EXPECT_EQ(0x01, outputBuffer[0][outputIndex]);		\
								EXPECT_EQ(0x01, outputBuffer[1][outputIndex]);		\
								EXPECT_EQ(0x04, outputBuffer[2][outputIndex]);		\
								outputIndex++;


#define TEST_PACKET_TWO_SMALL	EXPECT_EQ(0x01, outputBuffer[0][outputIndex]);		\
								EXPECT_EQ(0x01, outputBuffer[1][outputIndex]);		\
								EXPECT_EQ(0xAA, outputBuffer[2][outputIndex]);		\
								outputIndex++;
#define ADD_PACKET_ONE																\
	size += generateAFSKPrecise(0x00000000, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x1ACFFC1D, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x19AABBCC, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0xDDEEFF00, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x11223344, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x55667788, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x99AABBCC, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0xDDEEFF00, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x11220000, &preciseArray[size], 32, &phase);

#define ADD_PACKET_TWO																\
	size += generateAFSKPrecise(0x00000000, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x1ACFFC1D, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x19AABBCC, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0xDDEEFF00, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x11332244, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x55667788, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x99AABBCC, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0xDDEEFF00, &preciseArray[size], 32, &phase);		\
	size += generateAFSKPrecise(0x11220000, &preciseArray[size], 32, &phase);

#define RECEIVE_ONE_PACKET					\
	for(uint32_t i = 0; i < 200; i++)		\
	{										\
		UplinkExecute();					\
	}										\
	outputBufferCount = 0;
	

#define TEST_PACKET_ONE								    \
		EXPECT_EQ(0x1, outputBuffer[0][outputIndex]);	\
		EXPECT_EQ(0x19, outputBuffer[1][outputIndex]);	\
		EXPECT_EQ(0xAA, outputBuffer[2][outputIndex]);	\
		EXPECT_EQ(0xBB, outputBuffer[3][outputIndex]);	\
		EXPECT_EQ(0xCC, outputBuffer[4][outputIndex]);	\
		EXPECT_EQ(0xDD, outputBuffer[5][outputIndex]);	\
		EXPECT_EQ(0xEE, outputBuffer[6][outputIndex]);	\
		EXPECT_EQ(0xFF, outputBuffer[7][outputIndex]);	\
		EXPECT_EQ(0x00, outputBuffer[8][outputIndex]);	\
		EXPECT_EQ(0x11, outputBuffer[9][outputIndex]);	\
		EXPECT_EQ(0x22, outputBuffer[10][outputIndex]);	\
		EXPECT_EQ(0x33, outputBuffer[11][outputIndex]);	\
		EXPECT_EQ(0x44, outputBuffer[12][outputIndex]);	\
		EXPECT_EQ(0x55, outputBuffer[13][outputIndex]);	\
		EXPECT_EQ(0x66, outputBuffer[14][outputIndex]);	\
		EXPECT_EQ(0x77, outputBuffer[15][outputIndex]);	\
		EXPECT_EQ(0x88, outputBuffer[16][outputIndex]);	\
		EXPECT_EQ(0x99, outputBuffer[17][outputIndex]);	\
		EXPECT_EQ(0xAA, outputBuffer[18][outputIndex]);	\
		EXPECT_EQ(0xBB, outputBuffer[19][outputIndex]);	\
		EXPECT_EQ(0xCC, outputBuffer[20][outputIndex]);	\
		EXPECT_EQ(0xDD, outputBuffer[21][outputIndex]);	\
		EXPECT_EQ(0xEE, outputBuffer[22][outputIndex]);	\
		EXPECT_EQ(0xFF, outputBuffer[23][outputIndex]);	\
		EXPECT_EQ(0x00, outputBuffer[24][outputIndex]);	\
		EXPECT_EQ(0x11, outputBuffer[25][outputIndex]);	\
		EXPECT_EQ(0x22, outputBuffer[26][outputIndex]); \
		outputIndex++;

#define TEST_PACKET_TWO									\
		EXPECT_EQ(0x1, outputBuffer[0][outputIndex]);	\
		EXPECT_EQ(0x19, outputBuffer[1][outputIndex]);	\
		EXPECT_EQ(0xAA, outputBuffer[2][outputIndex]);	\
		EXPECT_EQ(0xBB, outputBuffer[3][outputIndex]);	\
		EXPECT_EQ(0xCC, outputBuffer[4][outputIndex]);	\
		EXPECT_EQ(0xDD, outputBuffer[5][outputIndex]);	\
		EXPECT_EQ(0xEE, outputBuffer[6][outputIndex]);	\
		EXPECT_EQ(0xFF, outputBuffer[7][outputIndex]);	\
		EXPECT_EQ(0x00, outputBuffer[8][outputIndex]);	\
		EXPECT_EQ(0x11, outputBuffer[9][outputIndex]);	\
		EXPECT_EQ(0x33, outputBuffer[10][outputIndex]);	\
		EXPECT_EQ(0x22, outputBuffer[11][outputIndex]);	\
		EXPECT_EQ(0x44, outputBuffer[12][outputIndex]);	\
		EXPECT_EQ(0x55, outputBuffer[13][outputIndex]);	\
		EXPECT_EQ(0x66, outputBuffer[14][outputIndex]);	\
		EXPECT_EQ(0x77, outputBuffer[15][outputIndex]);	\
		EXPECT_EQ(0x88, outputBuffer[16][outputIndex]);	\
		EXPECT_EQ(0x99, outputBuffer[17][outputIndex]);	\
		EXPECT_EQ(0xAA, outputBuffer[18][outputIndex]);	\
		EXPECT_EQ(0xBB, outputBuffer[19][outputIndex]);	\
		EXPECT_EQ(0xCC, outputBuffer[20][outputIndex]);	\
		EXPECT_EQ(0xDD, outputBuffer[21][outputIndex]);	\
		EXPECT_EQ(0xEE, outputBuffer[22][outputIndex]);	\
		EXPECT_EQ(0xFF, outputBuffer[23][outputIndex]);	\
		EXPECT_EQ(0x00, outputBuffer[24][outputIndex]);	\
		/*EXPECT_EQ(0x11, outputBuffer[24][outputIndex]);*/	\
		/*EXPECT_EQ(0x22, outputBuffer[25][outputIndex]);*/ \
		outputIndex++;

#define MAX_PACKET_DATA_SIZE 27
//extern void(*p_uplinkExecute)(void);

static uint32_t inputBuffer (float *);
static uint32_t generateAFSKSignal(uint32_t z_data, float * z_buffer, uint8_t z_size, float * z_phase);
static uint32_t generateAFSKPrecise(uint32_t z_data, float * z_buffer, uint8_t z_size, float * z_phase);
static void decimateAFSKsignal(void);
static uint32_t xQueueSendFAKE(QueueHandle_t z_queue, const void * z_data, uint32_t z_ticks);

static float markInput[DEMOD_CORRELATION_WINDOW_SIZE];
static float spaceInput[DEMOD_CORRELATION_WINDOW_SIZE];
static uint32_t outputBufferCount = 0;
float phase = 0;
uint16_t preamble = 0;

#define SIZE_OF_PACKET_BUFFER 3

#define TOTAL_SAMPLES 22050000
#define BAUD_RATE 1200
float preciseArray[TOTAL_SAMPLES];

static float testArray[TOTAL_SAMPLES];

static uint8_t mark = 1;

uint8_t outputBuffer[1000][1000];
uint32_t outputCount;
uint16_t outputIndex;

class Demodulator : public testing::Test
{
public:

	void SetUp()
	{
		phase = 0;
		preamble = 0;
		mark = 1;				// Need to set this up for every test.
		outputBufferCount = 0;	// Reset our output buffer index.

		memset(testArray, 0, sizeof(testArray));
		memset(preciseArray, 0, sizeof(preciseArray));

		UplinkInit();

		memset(outputBuffer, 0, sizeof(outputBuffer));
		outputCount = 0;
		outputIndex = 0;

		FFF_RESET_HISTORY();
	}

	virtual void TearDown() 
	{

	}
};

static uint32_t inputBuffer(float * z_sample)
{
	if(outputBufferCount < sizeof(testArray))
	{
		*z_sample = testArray[outputBufferCount];
		outputBufferCount++;
	}
	return 1;
}

static uint32_t generateAFSKPrecise(uint32_t z_data, float * z_buffer, uint8_t z_size, float * z_phase)
{
	uint32_t samplePeriod = TOTAL_SAMPLES / BAUD_RATE;	// Sample period is an integer number of array elements
	uint32_t count = 0;

	for(uint32_t samples = 0; samples < z_size; samples++)
	{
		if(!(z_data & (0x80000000 >> samples)))
		{
			mark = !mark;
		}

		for(uint32_t i = 0; i < samplePeriod; i++)
		{
			if(mark)
			{
				z_buffer[count] = cos(*z_phase);
				*z_phase += 2.0*3.14159265358979323846*DEMOD_FREQ_MARK/TOTAL_SAMPLES;
			}
			else
			{
				z_buffer[count] = cos(*z_phase);
				*z_phase += 2.0*3.14159265358979323846*DEMOD_FREQ_SPACE/TOTAL_SAMPLES;
			}
			count++;
		}
	}
	return (z_size * samplePeriod);
}

static void decimateAFSKsignal(void)
{
	for(uint32_t i = 0; i <= (TOTAL_SAMPLES / 1000); i++)
	{
		testArray[i] = preciseArray[i*1000];
	}
}

/***********************************************************/
/* Test initialisation									   */
/***********************************************************/
TEST_F(Demodulator, DemodSucceedsInitiationIfBufferFuncRegistered)
{
	RegisterUplinkInput(&inputBuffer);
	EXPECT_EQ(1, UplinkExecute());
}

TEST_F(Demodulator, TestWeCanFindTheSyncVector)
{
	RegisterUplinkInput(&inputBuffer);
	uint32_t size = generateAFSKPrecise(0xFFFFFFFF, &preciseArray[0], 32, &phase);
	size += generateAFSKPrecise(0x1ACFFC1D, &preciseArray[size], 32, &phase);
	size += generateAFSKPrecise(0x01000000, &preciseArray[size], 8, &phase);
	decimateAFSKsignal();

	for(uint32_t i = 0; i < 100; i++)
	{
		UplinkExecute();
	}

	EXPECT_EQ(1, GetUplinkPacketCount());
}

TEST_F(Demodulator, TestWeCanFindTwoSyncVectors)
{
	RegisterUplinkInput(&inputBuffer);

	uint32_t size = generateAFSKPrecise(0xFFFFFFFF, &preciseArray[0], 32, &phase);
	size += generateAFSKPrecise(0x1ACFFC1D, &preciseArray[size], 32, &phase);
	size += generateAFSKPrecise(0x01000000, &preciseArray[size], 32, &phase);
	size += generateAFSKPrecise(0x1ACFFC1D, &preciseArray[size], 32, &phase);
	size += generateAFSKPrecise(0x01000000, &preciseArray[size], 32, &phase);
	decimateAFSKsignal();

	for(uint32_t i = 0; i < 100; i++)
	{
		UplinkExecute();
	}

	EXPECT_EQ(2, GetUplinkPacketCount());
}

TEST_F(Demodulator, TestIncorrectPacketSizeIsRejected)
{
	RegisterUplinkInput(&inputBuffer);
	uint32_t size = generateAFSKPrecise(0xFFFFFFFF, &preciseArray[0], 32, &phase);
	size += generateAFSKPrecise(0x1ACFFC1D, &preciseArray[size], 32, &phase);
	size += generateAFSKPrecise(0x1A000000, &preciseArray[size], 8, &phase);
	decimateAFSKsignal();

	for(uint32_t i = 0; i < 10; i++)
	{
		UplinkExecute();
	}

	EXPECT_EQ(0, GetUplinkPacketCount());
}

TEST_F(Demodulator, TestLimitOfPacketSizeIsAccepted)
{
	RegisterUplinkInput(&inputBuffer);
	uint32_t size = generateAFSKPrecise(0xFFFFFFFF, &preciseArray[0], 32, &phase);
	size += generateAFSKPrecise(0x1ACFFC1D, &preciseArray[size], 32, &phase);
	size += generateAFSKPrecise(0x01000000, &preciseArray[size], 16, &phase);
	decimateAFSKsignal();

	for(uint32_t i = 0; i < 1000; i++)
	{
		UplinkExecute();
	}

	EXPECT_EQ(1, GetUplinkPacketCount());
}

TEST_F(Demodulator, TestWeCanReceiveAPacketAfterWeRejectOne)
{
	RegisterUplinkInput(&inputBuffer);
	uint32_t size = 0;

	size += generateAFSKPrecise(0xFFFFFFFF, &preciseArray[0], 32, &phase);
	size += generateAFSKPrecise(0x1ACFFC1D, &preciseArray[size], 32, &phase);
	size += generateAFSKPrecise(0x1A000000, &preciseArray[size], 32, &phase);	// 0x19 is the limit of packet size (25 bytes).
	size += generateAFSKPrecise(0x1ACFFC1D, &preciseArray[size], 32, &phase);
	size += generateAFSKPrecise(0x19000000, &preciseArray[size], 32, &phase);

	decimateAFSKsignal();

	for(uint32_t i = 0; i < 1000; i++)
	{
		UplinkExecute();
	}

	EXPECT_EQ(1, GetUplinkPacketCount());
}

static uint32_t xQueueSendFAKE(QueueHandle_t z_queue, const void * z_data, uint32_t z_ticks)
{
	for(uint32_t i = 0; i < MAX_PACKET_DATA_SIZE; i++)
	{
		outputBuffer[i][outputCount] = ((uint8_t*)z_data)[i];
	}
	outputCount++;
	return 1;
}

TEST_F(Demodulator, TestDataIsPlacedIntoPacketCorrectly)
{
	UPLINK_PACKET packet;
	uint32_t size = 0;
	xQueueSend_fake.custom_fake = &xQueueSendFAKE;
	GetCommandQueueHandle_fake.return_val = 1;
	UplinkInit();
	RegisterUplinkInput(&inputBuffer);
	ADD_PACKET_ONE;

	decimateAFSKsignal();

	for(uint32_t i = 0; i < 100; i++)
	{
		UplinkExecute();
	}
	TEST_PACKET_ONE;
}

TEST_F(Demodulator, TestReading100Packets)
{
	UPLINK_PACKET packet;
	uint32_t size = 0;
	RegisterUplinkInput(&inputBuffer);
	
	ADD_PACKET_ONE_SMALL;
	decimateAFSKsignal();

	for(uint32_t i = 0; i < 100; i++)
	{
		RECEIVE_ONE_PACKET;
		TEST_PACKET_ONE_SMALL;
	}
}

TEST_F(Demodulator, TestReading100AlternatingPackets)
{
	UPLINK_PACKET packet;
	uint32_t size = 0;
	RegisterUplinkInput(&inputBuffer);
	
	ADD_PACKET_ONE_SMALL;
	ADD_PACKET_TWO_SMALL;
	decimateAFSKsignal();

	for(uint32_t i = 0; i < 50; i++)
	{
		RECEIVE_ONE_PACKET;
		RECEIVE_ONE_PACKET;
		TEST_PACKET_ONE_SMALL;
		TEST_PACKET_TWO_SMALL;
	}
}

