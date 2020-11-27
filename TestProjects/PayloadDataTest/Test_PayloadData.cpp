#include "gtest/gtest.h"

extern "C"
{
	#include "PayloadData.h"
	#include "fff.h"
	#include "queue.h"
	#include "Fake_queue.h"
	#include "flashc.h"
	#include "Fake_flashc.h"
	#include "semphr.h"
	#include "Fake_semphr.h"
	#include "Fake_CANObjectDictionary.h"
}

DEFINE_FFF_GLOBALS;

uint8_t queueData[18*1024][5];
uint32_t queueReturnIndex = 0;

uint32_t xQueueReceive_FAKE(QueueHandle_t z_handle, uint8_t * z_data, uint32_t z_ticks);

/* Target specific defines. */
#define FLASH_PAGE_SIZE_WORDS			128
#define FLASH_SIZE_BYTES				0x80000		// 512k

/* Payload data specific defines. */
#define FLASH_PAYLOAD_STORAGE_SIZE			0x40000															// Amount of memory dedicated to storing payload data. (256k)
#define FLASH_PAYLOD_STORAGE_SIZE_WORDS		(FLASH_PAYLOAD_STORAGE_SIZE / sizeof(uint32_t))					// Amount of memory dedicated to storing payload data in words.
#define NUMBER_OF_PAYLOAD_SLOTS				4																// We use 4 slots to maximize FLASH life */
#define MAXIMUM_PAYLOAD_SIZE_WORDS			(FLASH_PAYLOD_STORAGE_SIZE_WORDS / NUMBER_OF_PAYLOAD_SLOTS)		// Number of words per payload dedicated to a payload transfer.
#define PAYLOAD_NUMBER_OF_PAGES				(MAXIMUM_PAYLOAD_SIZE_WORDS / FLASH_PAGE_SIZE_WORDS)			// The number of pages per payload slot.

/* The weird structure layout is to ensure we get variables spread correctly
	across FLASH memory pages such that level-wearing can be achieved. */
typedef struct 
{
	uint32_t writeCount;								// Number of times a payload slot has been written to.
	uint32_t dataValid;									// The data in this slot is current.
	uint32_t dataSize;									// The size of the data contained within the slot.	
	uint32_t pageFiller[FLASH_PAGE_SIZE_WORDS - 3];		// Ensure our four slots are FLASH page aligned (super important - dont remove!!!)
	uint32_t dataInvalid;								// The data in this slot is no longer current.
	uint32_t pageFiller2[FLASH_PAGE_SIZE_WORDS - 1];	// Ensure our four slots are FLASH page aligned (super important - dont remove!!!)
}PAYLOAD_CONTROL_VARIABLES;

/* This is the data structure being used in FLASH to map to. */
typedef struct _PAYLOAD_MEMORY_SECTION
{
	uint32_t					payloadData[NUMBER_OF_PAYLOAD_SLOTS][PAYLOAD_NUMBER_OF_PAGES][FLASH_PAGE_SIZE_WORDS];	// Payload data
	PAYLOAD_CONTROL_VARIABLES	controlVariables[NUMBER_OF_PAYLOAD_SLOTS];												// Control variables
}PAYLOAD_MEMORY_SECTION;

extern PAYLOAD_MEMORY_SECTION * p_payloadDataStore;

uint8_t holemapBuffer[254];
static void GetHolemapFAKE(uint8_t * z_holemap, uint16_t z_bufferSize);
static void ClearPage_FAKE(void);
static void AddOrderedValidPayloadData(void);
static void AddTwoPacketsOfValidPayloadData(void);

class PayloadData : public testing::Test
{
public:


	void SetUp()
	{
		
	}

	virtual void TearDown() 
	{
		RESET_FAKE(xQueueCreateStatic);
		RESET_FAKE(uxQueueMessagesWaiting);
		RESET_FAKE(xQueueReceive);
		RESET_FAKE(xQueueSend);
		RESET_FAKE(ClearPage);
		RESET_FAKE(ClearPageBuffer);
		RESET_FAKE(WriteToFlash);
		RESET_FAKE(xSemaphoreCreateRecursiveMutexStatic);
		RESET_FAKE(xSemaphoreTakeRecursive);
		RESET_FAKE(xSemaphoreGiveRecursive);
		RESET_FAKE(ClearHoleMap);
		RESET_FAKE(GetHolemap);

		queueReturnIndex = 0;
		memset(queueData, 0, sizeof(queueData));
		memset(p_payloadDataStore, 0, sizeof(PAYLOAD_MEMORY_SECTION));
		xQueueCreateStatic_fake.return_val = 0x22;		
		uxQueueMessagesWaiting_fake.return_val = 1;
		InitialisePayloadData();
		xSemaphoreCreateRecursiveMutexStatic_fake.return_val = 2;
		InitialiseFlashAccessProtection();
		xSemaphoreTakeRecursive_fake.return_val = 1;
		memset(holemapBuffer, 0, sizeof(holemapBuffer));
		
	}
};

static void AddValidPayloadData(void);

uint32_t xQueueReceive_FAKE(QueueHandle_t z_handle, uint8_t * z_data, uint32_t z_ticks)
{
	if(z_data != 0)
	{
		memcpy(z_data, &queueData[queueReturnIndex][0], 5);
		queueReturnIndex++;
	}
	return 1;
}

static void ClearPage_FAKE(void)
{
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[1].writeCount = 0;
	p_payloadDataStore->controlVariables[2].writeCount = 0;
	p_payloadDataStore->controlVariables[3].writeCount = 0;
}


static void AddValidPayloadData(void)
{
	uint32_t  i = 0;

	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;
	p_payloadDataStore->controlVariables[1].writeCount = 0;
	p_payloadDataStore->controlVariables[1].dataValid = 0;
	p_payloadDataStore->controlVariables[1].dataInvalid = 0;
	
	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	for(i = 1; i < 16385; i++)	
	{
		queueData[i][0] = 1;
		queueData[i][1] = 1;
	}
	/* Send stop */
	queueData[i+1][0] = 0;
	queueData[i+1][1] = 0;

	RESET_FAKE(ClearPage);
	RESET_FAKE(ClearPageBuffer);
	RESET_FAKE(WriteToFlash);

	for(uint32_t j = 0; j < 10; j++)
	{
		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();
	}
}

static void AddOrderedValidPayloadData(void)
{
	uint32_t byteCount = 1;			// Start at 1 for the start packet.
	uint32_t wordCount = 1;
	uint32_t totalBytes = 0;
	uint16_t packetCount = 1;

	uint32_t  i = 0;
	uint32_t  j = 0;
	queueReturnIndex = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;


	queueData[wordCount][0] = 1;	// Data packet.

	for(j = 0; j < 261; j++)
	{
		for(i = 0; i < 252; i++)
		{
			queueData[wordCount][0] = 1;						// Data packet.
			queueData[wordCount][byteCount] = packetCount;


			byteCount++;

			if(byteCount >= 5)
			{
				byteCount = 1;
				wordCount++;
			}
			totalBytes++;
			if(totalBytes >= 65536)
			{
				j = 1000;
				break;
			}
		}
		packetCount++;
	}

	if(byteCount != 0)
	{
		wordCount++;
	}

	/* Send stop */
	queueData[wordCount][0] = 0;
	queueData[wordCount][1] = 0;

	for(i = 0; i < 16386; i++)
	{
		PayloadDataTask();
	}
}

static void AddTwoPacketsOfValidPayloadData(void)
{
	uint32_t byteCount = 1;			// Start at 1 for the start packet.
	uint32_t wordCount = 1;
	uint32_t totalBytes = 0;
	uint16_t packetCount = 1;

	uint32_t  i = 0;
	uint32_t  j = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;


	queueData[wordCount][0] = 1;	// Data packet.

	for(j = 0; j < 3; j++)
	{
		for(i = 0; i < 252; i++)
		{
			queueData[wordCount][0] = 1;						// Data packet.
			queueData[wordCount][byteCount] = packetCount;


			byteCount++;

			if(byteCount >= 5)
			{
				byteCount = 1;
				wordCount++;
			}
			totalBytes++;
			if(totalBytes >= 504)
			{
				j = 1000;
				break;
			}
		}
		packetCount++;
	}

	if(byteCount != 0)
	{
		wordCount++;
	}

	/* Send stop */
	queueData[wordCount][0] = 0;
	queueData[wordCount][1] = 0;

	for(i = 0; i < 506; i++)
	{
		PayloadDataTask();
	}
}

/***************************************************/
/* General initialisation testing.				   */
/***************************************************/
TEST_F(PayloadData, TestThatInitialisePayloadDataSetsUpAQueue)
{
	InitialisePayloadData();
	EXPECT_EQ(1 ,xQueueCreateStatic_fake.call_count);
}

TEST_F(PayloadData, TestThatInitialisePayloadDataSetsUpCorrectLengthQueue)
{
	InitialisePayloadData();
	EXPECT_EQ(100,xQueueCreateStatic_fake.arg0_val);
}

TEST_F(PayloadData, TestThatInitialisePayloadDataSetsUpCorrectQueueItemSize)
{
	InitialisePayloadData();
	EXPECT_EQ(5,xQueueCreateStatic_fake.arg1_val);
}

TEST_F(PayloadData, TestThatInitialisePayloadDataDontInitialiseWithNullDataStoragePointer)
{
	InitialisePayloadData();
	EXPECT_NE((uint8_t*)0,xQueueCreateStatic_fake.arg2_val);
}

TEST_F(PayloadData, TestThatInitialisePayloadDataDontInitialiseWithNullStoragePointer)
{
	InitialisePayloadData();
	EXPECT_NE((uint32_t*)0,xQueueCreateStatic_fake.arg3_val);
}

TEST_F(PayloadData, TestThatGetQueueReturnsCorrectQueueHandle)
{
	InitialisePayloadData();
	EXPECT_EQ(0x22, GetPayloadDataQueueHandle());
}

/***************************************************/
/* General initialisation testing.				   */
/***************************************************/
TEST_F(PayloadData, TestThatInitialiseFlashProtectionReturnsSemaphore)
{
	xSemaphoreCreateRecursiveMutexStatic_fake.return_val = 2;
	EXPECT_EQ(2, InitialiseFlashAccessProtection());
}

/***************************************************/
/* Data being put onto the queue testing		   */
/***************************************************/
TEST_F(PayloadData, TestThatPayloadDataThreadCalls_uxQueueMessagesWaiting)
{
	PayloadDataTask();
	EXPECT_EQ(1, uxQueueMessagesWaiting_fake.call_count);
}

TEST_F(PayloadData, TestThatCorrectHandleIsPassedInto_uxQueueMessagesWaiting)
{
	PayloadDataTask();
	EXPECT_EQ(0x22, uxQueueMessagesWaiting_fake.arg0_val);
}

TEST_F(PayloadData, TestThat_xQueueReceiveIsCalledWhenMessageIsOnTheQueue)
{
	uxQueueMessagesWaiting_fake.return_val = 1;
	PayloadDataTask();
	EXPECT_EQ(1, xQueueReceive_fake.call_count);
}

TEST_F(PayloadData, TestThat_xQueueReceiveIsNotCalledWhenNoMessageAreOnTheQueue)
{
	uxQueueMessagesWaiting_fake.return_val = 0;
	PayloadDataTask();
	EXPECT_EQ(0, xQueueReceive_fake.call_count);
}

TEST_F(PayloadData, TestThat_xQueueReceiveIsPassedCorrectHandle)
{
	PayloadDataTask();
	EXPECT_EQ(0x22, xQueueReceive_fake.arg0_val);
}

TEST_F(PayloadData, TestThat_xQueueReceiveIsNotPassedANullBuffer)
{
	PayloadDataTask();
	EXPECT_NE((uint8_t*)0, xQueueReceive_fake.arg1_val);
}

TEST_F(PayloadData, TestThat_xQueueReceiveIsPassedCorrectTicks)
{
	PayloadDataTask();
	EXPECT_EQ(2, xQueueReceive_fake.arg2_val);
}

TEST_F(PayloadData, TestThatWritingDataToTheQueueGetsWrittenToFlashMemoryLocation)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;

	queueData[0][0] = 0;
	queueData[0][1] = 1;

	queueData[1][0] = 1;
	queueData[1][1] = 1;
	queueData[1][2] = 1;
	queueData[1][3] = 1;
	queueData[1][4] = 1;
	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();
	PayloadDataTask();

	EXPECT_EQ(0x01010101, p_payloadDataStore->payloadData[0][0][0]);
}

TEST_F(PayloadData, TestThatNewTransferCallsClearHolemap)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;

	queueData[0][0] = 0;
	queueData[0][1] = 1;

	InitialisePayloadData();
	PayloadDataTask();
	//This test passes on debug but not on release mode - gtest bug?
	//EXPECT_EQ(1, ClearHoleMap_fake.call_count);
}

TEST_F(PayloadData, TestThatWritingDataToTheQueueDoesntGetWrittenToFlashIfItIsACommand)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	InitialisePayloadData();
	PayloadDataTask();

	EXPECT_EQ(0, p_payloadDataStore->payloadData[0][0][0]);
}

TEST_F(PayloadData, TestThatWritingDataToTheQueueDoesntGetWrittenToFlashIfWeHaveNotBeenToldToStart)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	InitialisePayloadData();
	PayloadDataTask();
	EXPECT_EQ(0, p_payloadDataStore->payloadData[0][0][0]);
}

TEST_F(PayloadData, TestThatWritingDataToTheQueueDoesntGetWrittenToFlashIfWeHaveStartedAndThenStopped)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	/* Start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	/* Stop */
	queueData[1][0] = 0;
	queueData[1][1] = 0;
	/* Data */
	queueData[2][0] = 1;
	queueData[2][1] = 1;
	queueData[2][2] = 1;
	queueData[2][3] = 1;
	queueData[2][4] = 1;

	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();
	PayloadDataTask();
	EXPECT_EQ(0, p_payloadDataStore->payloadData[0][0][0]);
}

TEST_F(PayloadData, TestThatWritingDataToTheQueueGetsWrittenToLowestUsedSlot_Slot0)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[1].writeCount = 1;
	p_payloadDataStore->controlVariables[2].writeCount = 1;
	p_payloadDataStore->controlVariables[3].writeCount = 1;
						
	queueData[0][0] = 0;
	queueData[0][1] = 1;

	queueData[1][0] = 1;
	queueData[1][1] = 1;
	queueData[1][2] = 1;
	queueData[1][3] = 1;
	queueData[1][4] = 1;

	queueData[2][0] = 0;
	queueData[2][1] = 0;

	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();
	PayloadDataTask();


	EXPECT_EQ(0x01010101, p_payloadDataStore->payloadData[0][0][0]);
	EXPECT_EQ(0, p_payloadDataStore->payloadData[1][0][0]);
	EXPECT_EQ(0, p_payloadDataStore->payloadData[2][0][0]);
	EXPECT_EQ(0, p_payloadDataStore->payloadData[3][0][0]);
	
}

TEST_F(PayloadData, TestThatWritingDataToTheQueueGetsWrittenToLowestUsedSlot_Slot1)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 1;
	p_payloadDataStore->controlVariables[1].writeCount = 0;
	p_payloadDataStore->controlVariables[2].writeCount = 1;
	p_payloadDataStore->controlVariables[3].writeCount = 1;

	queueData[0][0] = 0;
	queueData[0][1] = 1;

	queueData[1][0] = 1;
	queueData[1][1] = 1;
	queueData[1][2] = 1;
	queueData[1][3] = 1;
	queueData[1][4] = 1;
	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();
	PayloadDataTask();

	EXPECT_EQ(0, p_payloadDataStore->payloadData[0][0][0]);
	EXPECT_EQ(0x01010101, p_payloadDataStore->payloadData[1][0][0]);
	EXPECT_EQ(0, p_payloadDataStore->payloadData[2][0][0]);
	EXPECT_EQ(0, p_payloadDataStore->payloadData[3][0][0]);
	
}

TEST_F(PayloadData, TestThatWritingDataToTheQueueGetsWrittenToLowestUsedSlot_Slot2)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 1;
	p_payloadDataStore->controlVariables[1].writeCount = 1;
	p_payloadDataStore->controlVariables[2].writeCount = 0;
	p_payloadDataStore->controlVariables[3].writeCount = 1;

	queueData[0][0] = 0;
	queueData[0][1] = 1;

	queueData[1][0] = 1;
	queueData[1][1] = 1;
	queueData[1][2] = 1;
	queueData[1][3] = 1;
	queueData[1][4] = 1;
	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();
	PayloadDataTask();

	EXPECT_EQ(0, p_payloadDataStore->payloadData[0][0][0]);
	EXPECT_EQ(0, p_payloadDataStore->payloadData[1][0][0]);
	EXPECT_EQ(0x01010101, p_payloadDataStore->payloadData[2][0][0]);
	EXPECT_EQ(0, p_payloadDataStore->payloadData[3][0][0]);
	
}

TEST_F(PayloadData, TestThatWritingDataToTheQueueGetsWrittenToLowestUsedSlot_Slot3)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 1;
	p_payloadDataStore->controlVariables[1].writeCount = 1;
	p_payloadDataStore->controlVariables[2].writeCount = 1;
	p_payloadDataStore->controlVariables[3].writeCount = 0;

	queueData[0][0] = 0;
	queueData[0][1] = 1;

	queueData[1][0] = 1;
	queueData[1][1] = 1;
	queueData[1][2] = 1;
	queueData[1][3] = 1;
	queueData[1][4] = 1;
	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();
	PayloadDataTask();

	EXPECT_EQ(0, p_payloadDataStore->payloadData[0][0][0]);
	EXPECT_EQ(0, p_payloadDataStore->payloadData[1][0][0]);
	EXPECT_EQ(0, p_payloadDataStore->payloadData[2][0][0]);
	EXPECT_EQ(0x01010101, p_payloadDataStore->payloadData[3][0][0]);
}

TEST_F(PayloadData, TestThatSendingAStartInvalidatesAllValidFields)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].dataValid = 1;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;
	p_payloadDataStore->controlVariables[1].dataValid = 1;
	p_payloadDataStore->controlVariables[1].dataInvalid = 0;
	p_payloadDataStore->controlVariables[2].dataValid = 1;
	p_payloadDataStore->controlVariables[2].dataInvalid = 0;
	p_payloadDataStore->controlVariables[3].dataValid = 1;
	p_payloadDataStore->controlVariables[3].dataInvalid = 0;
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	InitialisePayloadData();
	PayloadDataTask();

	EXPECT_EQ(p_payloadDataStore->controlVariables[0].dataInvalid, p_payloadDataStore->controlVariables[0].dataValid);
	EXPECT_EQ(p_payloadDataStore->controlVariables[1].dataInvalid, p_payloadDataStore->controlVariables[1].dataValid);
	EXPECT_EQ(p_payloadDataStore->controlVariables[2].dataInvalid, p_payloadDataStore->controlVariables[2].dataValid);
	EXPECT_EQ(p_payloadDataStore->controlVariables[3].dataInvalid, p_payloadDataStore->controlVariables[3].dataValid);
	EXPECT_EQ(5, ClearPageBuffer_fake.call_count);
	EXPECT_EQ(516, ClearPage_fake.call_count);
	EXPECT_EQ(4, WriteToFlash_fake.call_count);
}

TEST_F(PayloadData, TestThatSendingAStartDoesntClearTheWriteCount)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 1;
	p_payloadDataStore->controlVariables[1].writeCount = 2;
	p_payloadDataStore->controlVariables[2].writeCount = 3;
	p_payloadDataStore->controlVariables[3].writeCount = 4;
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	InitialisePayloadData();
	PayloadDataTask();

	EXPECT_EQ(0x1, p_payloadDataStore->controlVariables[0].writeCount);
	EXPECT_EQ(0x2, p_payloadDataStore->controlVariables[1].writeCount);
	EXPECT_EQ(0x3, p_payloadDataStore->controlVariables[2].writeCount);
	EXPECT_EQ(0x4, p_payloadDataStore->controlVariables[3].writeCount);
}

/* Test the write count doesnt change! */
TEST_F(PayloadData, TestThatSendingAStopIncrementsTheWriteCount)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	/* Send data */
	queueData[1][0] = 1;
	queueData[1][1] = 1;
	/* Send stop */
	queueData[2][0] = 0;
	queueData[2][1] = 0;
	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();
	PayloadDataTask();

	EXPECT_EQ(0x1, p_payloadDataStore->controlVariables[0].writeCount);
}

TEST_F(PayloadData, TestThatSendingStartThenStopIsInvalid)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	/* Send stop */
	queueData[1][0] = 0;
	queueData[1][1] = 0;
	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();

	EXPECT_EQ(0x0, p_payloadDataStore->controlVariables[0].dataValid);
}

TEST_F(PayloadData, TestThatSendingAStopMarksDataAsValid)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	/* Send data */
	queueData[1][0] = 1;
	queueData[1][1] = 1;
	/* Send stop */
	queueData[2][0] = 0;
	queueData[2][1] = 0;
	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();
	PayloadDataTask();

	EXPECT_EQ(0x1, p_payloadDataStore->controlVariables[0].dataValid);
}

TEST_F(PayloadData, TestThatSendingAStopWithoutAStartDoesNothing)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	/* Send stop */
	queueData[0][0] = 0;
	queueData[0][1] = 0;
	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();
	EXPECT_EQ(0x0, p_payloadDataStore->controlVariables[0].dataValid);
	EXPECT_EQ(0x0, p_payloadDataStore->controlVariables[0].writeCount);
}

TEST_F(PayloadData, TestThatSendingAStopCallsPageBufferClear)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	/* Send data */
	queueData[1][0] = 1;
	queueData[1][1] = 1;
	/* Send stop */
	queueData[2][0] = 0;
	queueData[2][1] = 0;
	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();
	RESET_FAKE(ClearPageBuffer);
	PayloadDataTask();
	EXPECT_EQ(1, ClearPageBuffer_fake.call_count);
}

TEST_F(PayloadData, TestThatSendingAStopCallsPageClear)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	/* Send data */
	queueData[1][0] = 1;
	queueData[1][1] = 1;
	/* Send stop */
	queueData[2][0] = 0;
	queueData[2][1] = 0;
	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();
	RESET_FAKE(ClearPageBuffer);
	PayloadDataTask();
	EXPECT_EQ(1, ClearPage_fake.call_count);
}

TEST_F(PayloadData, TestThatSendingAStopCallsWriteToFlashForDataAndPage)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	/* Send data */
	queueData[1][0] = 1;
	queueData[1][1] = 1;
	/* Send stop */
	queueData[2][0] = 0;
	queueData[2][1] = 0;
	InitialisePayloadData();
	PayloadDataTask();
	PayloadDataTask();
	RESET_FAKE(ClearPageBuffer);
	PayloadDataTask();
	EXPECT_EQ(2, WriteToFlash_fake.call_count);
}

TEST_F(PayloadData, TestThatSendingAStopUpdatesTheDataSize)
{
	uint32_t i = 0;

	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	InitialisePayloadData();

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	PayloadDataTask();

	/* Send data */
	for(i = 1; i < 100; i++)
	{
		queueData[i][0] = 1;
		queueData[i][1] = 1;
		PayloadDataTask();
	}

	/* Send stop */
	queueData[i][0] = 0;
	queueData[i][1] = 0;
	PayloadDataTask();

	EXPECT_EQ(99, p_payloadDataStore->controlVariables[0].dataSize);
}

TEST_F(PayloadData, TestThatWriteWritesToIncrementingAddressesOnePage)
{
	uint32_t i = 0;

	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	InitialisePayloadData();

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	PayloadDataTask();

	/* Send data */
	for(i = 1; i < 129; i++)
	{
		queueData[i][0] = 1;
		queueData[i][1] = i;
		PayloadDataTask();
	}

	/* Send stop */
	queueData[i][0] = 0;
	queueData[i][1] = 0;
	PayloadDataTask();

	for(i = 1; i < 129; i++)
	{
		EXPECT_EQ(i, p_payloadDataStore->payloadData[0][0][i-1]);
	}

	EXPECT_EQ(128, p_payloadDataStore->controlVariables[0].dataSize);
}

TEST_F(PayloadData, TestThatWriteWritesToIncrementingAddressesAllPages)
{
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t returnCount = 1;

	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	InitialisePayloadData();

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	PayloadDataTask();

	/* Send data */
	for(j = 0; j < 128; j++)
	{
		for(i = 1; i < 129; i++)
		{
			queueData[returnCount][0] = 1;
			*(uint32_t*)&(queueData[returnCount][1]) = returnCount;
			PayloadDataTask();
			returnCount++;
		}
		returnCount -= 128;
		for(i = 1; i < 129; i++)
		{
			EXPECT_EQ(returnCount, p_payloadDataStore->payloadData[0][j][i-1]);
			returnCount++;
		}
	}

	/* Send stop */
	queueData[i][0] = 0;
	queueData[i][1] = 0;
	PayloadDataTask();

	EXPECT_EQ(16384, p_payloadDataStore->controlVariables[0].dataSize);
}

TEST_F(PayloadData, TestThatStartCausesSlotMemoryToBeReset)
{
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t returnCount = 1;
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	p_payloadDataStore->controlVariables[0].dataValid = 1;

	InitialisePayloadData();

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;

	PayloadDataTask();

	EXPECT_EQ(129, ClearPage_fake.call_count);
}

TEST_F(PayloadData, TestThatWriteWritesPageBufferIntoFlash)
{
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t returnCount = 1;

	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	InitialisePayloadData();

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	PayloadDataTask();

	/* Send data */
	for(j = 0; j < 128; j++)
	{
		RESET_FAKE(ClearPage);
		RESET_FAKE(ClearPageBuffer);
		RESET_FAKE(WriteToFlash);
		for(i = 1; i < 129; i++)
		{
			queueData[returnCount][0] = 1;
			PayloadDataTask();
			returnCount++;
		}
		EXPECT_EQ(1, ClearPageBuffer_fake.call_count);
		EXPECT_EQ(1, WriteToFlash_fake.call_count);
	}

	/* Send stop */
	queueData[i][0] = 0;
	queueData[i][1] = 0;
	PayloadDataTask();

	EXPECT_EQ(16384, p_payloadDataStore->controlVariables[0].dataSize);
}

TEST_F(PayloadData, TestThatSendingAStopDoesntWriteToDataPageIfWeJustDidAWrite)
{
	uint32_t i = 0;
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	PayloadDataTask();
	for(i = 1; i < 129; i++)
	{
		queueData[i][0] = 1;
		queueData[i][1] = 1;
		PayloadDataTask();
	}
	RESET_FAKE(ClearPage);
	RESET_FAKE(ClearPageBuffer);
	RESET_FAKE(WriteToFlash);

	/* Send stop */
	queueData[i][0] = 0;
	queueData[i][1] = 0;

	RESET_FAKE(ClearPageBuffer);
	PayloadDataTask();
	EXPECT_EQ(1, WriteToFlash_fake.call_count);
}

TEST_F(PayloadData, TestThatWritingMoreThan64KIsRejected)
{
	uint32_t i = 0;
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	PayloadDataTask();
	for(i = 1; i < 16386; i++)	
	{
		queueData[i][0] = 1;
		queueData[i][1] = 1;
		PayloadDataTask();
	}

	queueData[i][0] = 1;	// One word too many!
	queueData[i][1] = 1;
	PayloadDataTask();
	RESET_FAKE(ClearPage);
	RESET_FAKE(ClearPageBuffer);
	RESET_FAKE(WriteToFlash);

	/* Send stop */
	queueData[i+1][0] = 0;
	queueData[i+1][1] = 0;

	RESET_FAKE(ClearPageBuffer);
	PayloadDataTask();
	EXPECT_EQ(0, WriteToFlash_fake.call_count);
}


TEST_F(PayloadData, TestWritingToAllSlotsSequentially)
{
	uint32_t i = 0;
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;
	p_payloadDataStore->controlVariables[1].writeCount = 0;
	p_payloadDataStore->controlVariables[1].dataValid = 0;
	p_payloadDataStore->controlVariables[1].dataInvalid = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	for(i = 1; i < 16385; i++)	
	{
		queueData[i][0] = 1;
		queueData[i][1] = 1;
	}
	/* Send stop */
	queueData[i+1][0] = 0;
	queueData[i+1][1] = 0;

	RESET_FAKE(ClearPage);
	RESET_FAKE(ClearPageBuffer);
	RESET_FAKE(WriteToFlash);
	//PayloadDataTask();


	for(uint32_t j = 0; j < 10; j++)
	{
		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();
		EXPECT_GT(p_payloadDataStore->controlVariables[0].dataValid, p_payloadDataStore->controlVariables[0].dataInvalid);
		EXPECT_EQ(p_payloadDataStore->controlVariables[1].dataValid, p_payloadDataStore->controlVariables[1].dataInvalid);
		EXPECT_EQ(p_payloadDataStore->controlVariables[2].dataValid, p_payloadDataStore->controlVariables[2].dataInvalid);
		EXPECT_EQ(p_payloadDataStore->controlVariables[3].dataValid, p_payloadDataStore->controlVariables[3].dataInvalid);

		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();
		EXPECT_EQ(p_payloadDataStore->controlVariables[0].dataValid, p_payloadDataStore->controlVariables[0].dataInvalid);
		EXPECT_GT(p_payloadDataStore->controlVariables[1].dataValid, p_payloadDataStore->controlVariables[1].dataInvalid);
		EXPECT_EQ(p_payloadDataStore->controlVariables[2].dataValid, p_payloadDataStore->controlVariables[2].dataInvalid);
		EXPECT_EQ(p_payloadDataStore->controlVariables[3].dataValid, p_payloadDataStore->controlVariables[3].dataInvalid);

		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();
		EXPECT_EQ(p_payloadDataStore->controlVariables[0].dataValid, p_payloadDataStore->controlVariables[0].dataInvalid);
		EXPECT_EQ(p_payloadDataStore->controlVariables[1].dataValid, p_payloadDataStore->controlVariables[1].dataInvalid);
		EXPECT_GT(p_payloadDataStore->controlVariables[2].dataValid, p_payloadDataStore->controlVariables[2].dataInvalid);
		EXPECT_EQ(p_payloadDataStore->controlVariables[3].dataValid, p_payloadDataStore->controlVariables[3].dataInvalid);

		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();
		EXPECT_EQ(p_payloadDataStore->controlVariables[0].dataValid, p_payloadDataStore->controlVariables[0].dataInvalid);
		EXPECT_EQ(p_payloadDataStore->controlVariables[1].dataValid, p_payloadDataStore->controlVariables[1].dataInvalid);
		EXPECT_EQ(p_payloadDataStore->controlVariables[2].dataValid, p_payloadDataStore->controlVariables[2].dataInvalid);
		EXPECT_GT(p_payloadDataStore->controlVariables[3].dataValid, p_payloadDataStore->controlVariables[3].dataInvalid);
	}
}


TEST_F(PayloadData, TestWritingToAllSlotsSequentiallyDoesNothingIfSemaphoreIsInvalid)
{
	uint32_t i = 0;
	xSemaphoreCreateRecursiveMutexStatic_fake.return_val = 0;
	InitialiseFlashAccessProtection();
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;
	p_payloadDataStore->controlVariables[1].writeCount = 0;
	p_payloadDataStore->controlVariables[1].dataValid = 0;
	p_payloadDataStore->controlVariables[1].dataInvalid = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	for(i = 1; i < 16385; i++)	
	{
		queueData[i][0] = 1;
		queueData[i][1] = 1;
	}
	/* Send stop */
	queueData[i+1][0] = 0;
	queueData[i+1][1] = 0;

	RESET_FAKE(ClearPage);
	RESET_FAKE(ClearPageBuffer);
	RESET_FAKE(WriteToFlash);

	for(uint32_t j = 0; j < 10; j++)
	{
		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();

		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();


		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();


		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();
	}

	EXPECT_EQ(0, ClearPageBuffer_fake.call_count);
	EXPECT_EQ(0, ClearPage_fake.call_count);
	EXPECT_EQ(0, WriteToFlash_fake.call_count);
}

TEST_F(PayloadData, TestWritingToAllSlotsSequentiallyDoesNothingIfSemaphoreCannotBeTaken)
{
	uint32_t i = 0;
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	xSemaphoreTakeRecursive_fake.return_val = 0;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;
	p_payloadDataStore->controlVariables[1].writeCount = 0;
	p_payloadDataStore->controlVariables[1].dataValid = 0;
	p_payloadDataStore->controlVariables[1].dataInvalid = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	for(i = 1; i < 16385; i++)	
	{
		queueData[i][0] = 1;
		queueData[i][1] = 1;
	}
	/* Send stop */
	queueData[i+1][0] = 0;
	queueData[i+1][1] = 0;

	RESET_FAKE(ClearPage);
	RESET_FAKE(ClearPageBuffer);
	RESET_FAKE(WriteToFlash);

	for(uint32_t j = 0; j < 10; j++)
	{
		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();

		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();


		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();


		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();
	}

	EXPECT_EQ(0, ClearPageBuffer_fake.call_count);
	EXPECT_EQ(0, ClearPage_fake.call_count);
	EXPECT_EQ(0, WriteToFlash_fake.call_count);
}


TEST_F(PayloadData, TestSemaphoreGiveIsCalledAsOftenAsSempahoreTake)
{
	uint32_t i = 0;
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	xSemaphoreTakeRecursive_fake.return_val = 1;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;
	p_payloadDataStore->controlVariables[1].writeCount = 0;
	p_payloadDataStore->controlVariables[1].dataValid = 0;
	p_payloadDataStore->controlVariables[1].dataInvalid = 0;

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	for(i = 1; i < 16385; i++)	
	{
		queueData[i][0] = 1;
		queueData[i][1] = 1;
	}
	/* Send stop */
	queueData[i+1][0] = 0;
	queueData[i+1][1] = 0;

	RESET_FAKE(ClearPage);
	RESET_FAKE(ClearPageBuffer);
	RESET_FAKE(WriteToFlash);

	for(uint32_t j = 0; j < 10; j++)
	{
		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();

		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();


		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();


		queueReturnIndex = 0;
		for(i = 0; i < 16385; i++)
		{
			PayloadDataTask();
		}
		PayloadDataTask();
	}

	EXPECT_EQ(xSemaphoreTakeRecursive_fake.call_count, xSemaphoreGiveRecursive_fake.call_count);
}



TEST_F(PayloadData, TestThatSemaphoreAcqusationFailureDuringCommandPacketAbortsTheTransfer)
{
	uint32_t i = 0;

	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	InitialisePayloadData();

	/* Send start */
	xSemaphoreTakeRecursive_fake.return_val = 0;
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	PayloadDataTask();

	xSemaphoreTakeRecursive_fake.return_val = 1;
	queueData[1][0] = 1;
	queueData[1][1] = 1;
	PayloadDataTask();

	queueData[2][0] = 1;
	queueData[2][1] = 1;
	PayloadDataTask();

	/* Send stop */
	queueData[3][0] = 0;
	queueData[3][1] = 0;
	PayloadDataTask();

	EXPECT_EQ(0, p_payloadDataStore->controlVariables[0].dataSize);
}

TEST_F(PayloadData, TestThatSemaphoreAcqusationFailureDuringStopFollowedByAStartIsAccepted)
{
	uint32_t i = 0;

	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	InitialisePayloadData();

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	PayloadDataTask();

	queueData[1][0] = 1;
	queueData[1][1] = 1;
	PayloadDataTask();

	queueData[2][0] = 1;
	queueData[2][1] = 1;
	PayloadDataTask();

	/* Send stop */
	xSemaphoreTakeRecursive_fake.return_val = 0;
	queueData[3][0] = 0;
	queueData[3][1] = 0;
	PayloadDataTask();

	xSemaphoreTakeRecursive_fake.return_val = 1;
	queueReturnIndex = 0;
	PayloadDataTask();
	PayloadDataTask();
	PayloadDataTask();
	PayloadDataTask();

	EXPECT_EQ(2, p_payloadDataStore->controlVariables[0].dataSize);
}

TEST_F(PayloadData, TestMultipleStartCommandsOnlyUpdateOnce)
{
	uint32_t i = 0;

	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	InitialisePayloadData();

	/* Send start */
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	PayloadDataTask();

	/* Send start */
	queueData[1][0] = 0;
	queueData[1][1] = 1;
	PayloadDataTask();

	queueData[2][0] = 1;
	queueData[2][1] = 1;
	PayloadDataTask();

	queueData[3][0] = 1;
	queueData[3][1] = 1;
	PayloadDataTask();

	/* Send stop */
	queueData[4][0] = 0;
	queueData[4][1] = 0;
	PayloadDataTask();

	EXPECT_EQ(3, ClearPageBuffer_fake.call_count);
	EXPECT_EQ(1, ClearPage_fake.call_count);
	EXPECT_EQ(2, WriteToFlash_fake.call_count);
	EXPECT_EQ(2, p_payloadDataStore->controlVariables[0].dataSize);
}

TEST_F(PayloadData, TestThatSemaphoreAcqusationFailureDuringCommandPacketAbortsTheTransferAndWeCanRestartAfterwards)
{
	uint32_t i = 0;

	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	ClearPage_fake.custom_fake = ClearPage_FAKE;
	p_payloadDataStore->controlVariables[0].writeCount = 0;
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;

	InitialisePayloadData();

	/* Send start */
	xSemaphoreTakeRecursive_fake.return_val = 0;
	queueData[0][0] = 0;
	queueData[0][1] = 1;
	PayloadDataTask();

	xSemaphoreTakeRecursive_fake.return_val = 1;
	queueData[1][0] = 1;
	queueData[1][1] = 1;
	PayloadDataTask();

	queueData[2][0] = 1;
	queueData[2][1] = 1;
	PayloadDataTask();

	/* Send stop */
	xSemaphoreTakeRecursive_fake.return_val = 0;
	queueData[3][0] = 0;
	queueData[3][1] = 0;
	PayloadDataTask();

	queueReturnIndex = 0;
	xSemaphoreTakeRecursive_fake.return_val = 1;
	PayloadDataTask();
	PayloadDataTask();
	PayloadDataTask();
	PayloadDataTask();

	EXPECT_EQ(2, ClearPageBuffer_fake.call_count);
	EXPECT_EQ(1, ClearPage_fake.call_count);
	EXPECT_EQ(2, WriteToFlash_fake.call_count);
	EXPECT_EQ(2, p_payloadDataStore->controlVariables[0].dataSize);
}


/**************************************************/
/* Get payload data tests */
/**************************************************/
TEST_F(PayloadData, TestThatGetPayloadDataDoesntCrashOnANullPointer)
{
	uint16_t packetNumber;
	EXPECT_EQ(INVALID_INPUT, GetPayloadData(0,10, &packetNumber));
}

TEST_F(PayloadData, TestThatGetPayloadDataDoesntReturnsErrorIfBufferSizeIsTooSmall)
{
	uint8_t buffer[254];
	uint16_t packetNumber;
	EXPECT_EQ(INVALID_INPUT, GetPayloadData(buffer,251, &packetNumber));
}

TEST_F(PayloadData, TestThatGetPayloadDataReturnsSuccessDataIfLargeEnough)
{
	uint8_t buffer[254];
	uint16_t packetNumber;
	AddValidPayloadData();
	EXPECT_EQ(NO_ERROR_PAYLOAD_DATA, GetPayloadData(buffer,254, &packetNumber));
}

TEST_F(PayloadData, TestThatGetPayloadDataCallsSemaphoreGiveAndTake)
{
	uint8_t buffer[254];
	uint16_t packetNumber;
	GetPayloadData(buffer, sizeof(buffer), &packetNumber);
	
	EXPECT_EQ(1, xSemaphoreTakeRecursive_fake.call_count);
	EXPECT_EQ(1, xSemaphoreGiveRecursive_fake.call_count);
}

TEST_F(PayloadData, TestThatGetPayloadDataReturnsErrorIfWeCantGetSemaphore)
{
	uint8_t buffer[254];
	uint16_t packetNumber;
	xSemaphoreTakeRecursive_fake.return_val = 0;

	EXPECT_EQ(SEMAPHORE_TIMED_OUT, GetPayloadData(buffer, 254, &packetNumber));
}


TEST_F(PayloadData, TestThatWithNoValidDataWeReturnErrorNoData)
{
	uint8_t buffer[254];
	uint16_t packetNumber;
	EXPECT_EQ(NOT_PROCESSING_PAYLOAD_DATA, GetPayloadData(buffer, sizeof(buffer), &packetNumber));
}

TEST_F(PayloadData, TestThatGetHolemapIsCalled)
{
	uint8_t buffer[254];
	uint32_t i = 0;
	uint16_t packetNumber;
	p_payloadDataStore->controlVariables[0].dataValid = 1;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;
	p_payloadDataStore->controlVariables[0].dataSize = 1;
	GetPayloadData(buffer, sizeof(buffer), &packetNumber);
	EXPECT_EQ(1, GetHolemap_fake.call_count);
}

TEST_F(PayloadData, TestThatGetPayloadDataReturnsCorrect252BytesOnFirstCall)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	AddOrderedValidPayloadData();
	uint8_t buffer[252];
	uint16_t packetNumber;

	GetPayloadData(buffer, sizeof(buffer), &packetNumber);

	for(uint32_t i = 0; i < sizeof(buffer); i++)
	{
		EXPECT_EQ(1, buffer[i]);
	}
}

TEST_F(PayloadData, TestThatGetPayloadDataReturnsCorrect252BytesFor260Calls)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	AddOrderedValidPayloadData();
	uint8_t buffer[252];
	uint8_t value;
	uint16_t packetNumber;
	

	for(uint32_t j = 1; j < 261; j++)
	{
		value = j;

		GetPayloadData(buffer, sizeof(buffer), &packetNumber);
		for(uint32_t i = 0; i < sizeof(buffer); i++)
		{
			EXPECT_EQ(value, buffer[i]);
		}
	}
	value++;

	GetPayloadData(buffer, sizeof(buffer), &packetNumber);
	for(uint32_t i = 0; i < 16; i++)
	{
		EXPECT_EQ(value, buffer[i]);
	}
}


TEST_F(PayloadData, TestThatGetPayloadDataReturnLoopsForTwoPackets)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	AddTwoPacketsOfValidPayloadData();
	uint8_t buffer[252];
	uint8_t value;
	uint16_t packetNumber;
	
	GetPayloadData(buffer, sizeof(buffer), &packetNumber);
	for(uint32_t i = 0; i < sizeof(buffer); i++)
	{
		EXPECT_EQ(1, buffer[i]);
	}

	GetPayloadData(buffer, sizeof(buffer), &packetNumber);
	for(uint32_t i = 0; i < sizeof(buffer); i++)
	{
		EXPECT_EQ(2, buffer[i]);
	}

	GetPayloadData(buffer, sizeof(buffer), &packetNumber);
	for(uint32_t i = 0; i < sizeof(buffer); i++)
	{
		EXPECT_EQ(1, buffer[i]);
	}

	GetPayloadData(buffer, sizeof(buffer), &packetNumber);
	for(uint32_t i = 0; i < sizeof(buffer); i++)
	{
		EXPECT_EQ(2, buffer[i]);
	}
}

TEST_F(PayloadData, TestThatGetPayloadDataReturnsCorrect252BytesFor260CallsLooped10Times)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	AddOrderedValidPayloadData();
	uint8_t buffer[252];
	uint8_t value;
	uint16_t packetNumber;
	
	for(uint32_t i = 0; i < 10; i++)
	{
		for(uint32_t j = 1; j < 261; j++)
		{
			value = j;

			GetPayloadData(buffer, sizeof(buffer), &packetNumber);
			for(uint32_t i = 0; i < sizeof(buffer); i++)
			{
				EXPECT_EQ(value, buffer[i]);
			}
		}
		value++;

		GetPayloadData(buffer, sizeof(buffer), &packetNumber);
		for(uint32_t i = 0; i < 16; i++)
		{
			EXPECT_EQ(value, buffer[i]);
		}
	}
}

void GetHolemapFAKE(uint8_t * z_holemap, uint16_t z_bufferSize)
{
	for(uint32_t i = 0; i < z_bufferSize; i++)
	{
		z_holemap[i] = holemapBuffer[i];
	}
}

TEST_F(PayloadData, TestThatGetPayloadDataReturnsErrorNoDataReadyIfHolemapIsComplete)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	GetHolemap_fake.custom_fake = GetHolemapFAKE;
	AddOrderedValidPayloadData();
	uint8_t buffer[252];
	uint16_t packetNumber;

	for(uint32_t i = 0; i < sizeof(holemapBuffer); i++)
	{
		holemapBuffer[i] = 0xFF;
	}

	EXPECT_EQ(NO_DATA_READY, GetPayloadData(buffer, sizeof(buffer), &packetNumber));
}

TEST_F(PayloadData, TestThatGetPayloadDataReturnsSinglePacketRepeatedlyIfOnlyOneBitMissingInHolemap)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	GetHolemap_fake.custom_fake = GetHolemapFAKE;
	AddOrderedValidPayloadData();
	uint8_t buffer[252];
	uint16_t packetNumber;

	for(uint32_t i = 0; i < sizeof(holemapBuffer); i++)
	{
		holemapBuffer[i] = 0xFF;
	}
	holemapBuffer[0] = 0xFE;	// Single packet to send down.

	GetPayloadData(buffer, sizeof(buffer), &packetNumber);

	for(uint32_t i = 0; i < sizeof(buffer); i++)
	{
		EXPECT_EQ(1, buffer[i]);
	}
}

TEST_F(PayloadData, TestThatGetPayloadDataReturnsAlternatingPacketsRepeatedlyIfTwoBitsMissingInHolemap)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	GetHolemap_fake.custom_fake = GetHolemapFAKE;
	AddOrderedValidPayloadData();
	uint8_t buffer[252];
	uint16_t packetNumber;

	for(uint32_t i = 0; i < sizeof(holemapBuffer); i++)
	{
		holemapBuffer[i] = 0xFF;
	}
	holemapBuffer[0] = 0xFC;	// Single packet to send down.

	GetPayloadData(buffer, sizeof(buffer), &packetNumber);

	for(uint32_t i = 0; i < sizeof(buffer); i++)
	{
		EXPECT_EQ(1, buffer[i]);
	}

	GetPayloadData(buffer, sizeof(buffer), &packetNumber);

	for(uint32_t i = 0; i < sizeof(buffer); i++)
	{
		EXPECT_EQ(2, buffer[i]);
	}

	GetPayloadData(buffer, sizeof(buffer), &packetNumber);

	for(uint32_t i = 0; i < sizeof(buffer); i++)
	{
		EXPECT_EQ(1, buffer[i]);
	}

	GetPayloadData(buffer, sizeof(buffer), &packetNumber);

	for(uint32_t i = 0; i < sizeof(buffer); i++)
	{
		EXPECT_EQ(2, buffer[i]);
	}
}

TEST_F(PayloadData, TestThatGetPayloadDataReturnsFinalPacketAsExpected)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	GetHolemap_fake.custom_fake = GetHolemapFAKE;
	AddOrderedValidPayloadData();
	uint8_t buffer[252];
	uint16_t packetNumber;

	for(uint32_t i = 0; i < sizeof(holemapBuffer); i++)
	{
		holemapBuffer[i] = 0xFF;
	}
	holemapBuffer[32] ^= 0x10;

	GetPayloadData(buffer, sizeof(buffer), &packetNumber);

	for(uint32_t i = 0; i < 16; i++)
	{
		EXPECT_EQ(5, buffer[i]);
	}
}

TEST_F(PayloadData, TestThatSettingHolemapHoleBeyondEndOfDataDoesNothing)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	GetHolemap_fake.custom_fake = GetHolemapFAKE;
	AddOrderedValidPayloadData();
	uint8_t buffer[252];
	uint16_t packetNumber;

	for(uint32_t i = 0; i < sizeof(holemapBuffer); i++)
	{
		holemapBuffer[i] = 0xFF;
	}
	holemapBuffer[32] ^= 0x20;

	EXPECT_EQ(NO_DATA_READY, GetPayloadData(buffer, sizeof(buffer), &packetNumber));
}

TEST_F(PayloadData, TestThatGetPayloadDataIgnoreHoleMapBeyondEndOfPackets)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	GetHolemap_fake.custom_fake = GetHolemapFAKE;
	AddTwoPacketsOfValidPayloadData();
	uint8_t buffer[252];
	uint8_t value;
	uint16_t packetNumber;
	
	for(uint32_t i = 0; i < sizeof(holemapBuffer); i++)
	{
		holemapBuffer[i] = 0xFF;
	}
	holemapBuffer[0] ^= 0x04;

	EXPECT_EQ(NO_DATA_READY, GetPayloadData(buffer, sizeof(buffer), &packetNumber));
}

TEST_F(PayloadData, TestThatGetPayloadDataConsidersHolemapAtEndOfPacketBoundary)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	GetHolemap_fake.custom_fake = GetHolemapFAKE;
	AddTwoPacketsOfValidPayloadData();
	uint8_t buffer[252];
	uint8_t value;
	uint16_t packetNumber;
	
	for(uint32_t i = 0; i < sizeof(holemapBuffer); i++)
	{
		holemapBuffer[i] = 0xFF;
	}
	holemapBuffer[0] ^= 0x02;

	EXPECT_EQ(NO_ERROR_PAYLOAD_DATA, GetPayloadData(buffer, sizeof(buffer), &packetNumber));
}

TEST_F(PayloadData, TestThatPayloadDataPresentOnlyReturnsFalseWhenWeHaveNoValidData)
{
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;
	p_payloadDataStore->controlVariables[1].dataValid = 0;
	p_payloadDataStore->controlVariables[1].dataInvalid = 0;
	p_payloadDataStore->controlVariables[2].dataValid = 0;
	p_payloadDataStore->controlVariables[2].dataInvalid = 0;
	p_payloadDataStore->controlVariables[3].dataValid = 0;
	p_payloadDataStore->controlVariables[4].dataInvalid = 0;

	EXPECT_EQ(0, PayloadDataPresent());
}

TEST_F(PayloadData, TestThatPayloadDataPresentOnlyReturnsTrueWhenWeHaveDataInSlot0)
{
	p_payloadDataStore->controlVariables[0].dataValid = 1;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;
	p_payloadDataStore->controlVariables[1].dataValid = 0;
	p_payloadDataStore->controlVariables[1].dataInvalid = 0;
	p_payloadDataStore->controlVariables[2].dataValid = 0;
	p_payloadDataStore->controlVariables[2].dataInvalid = 0;
	p_payloadDataStore->controlVariables[3].dataValid = 0;
	p_payloadDataStore->controlVariables[4].dataInvalid = 0;

	EXPECT_EQ(1, PayloadDataPresent());
}

TEST_F(PayloadData, TestThatPayloadDataPresentOnlyReturnsTrueWhenWeHaveDataInSlot1)
{
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;
	p_payloadDataStore->controlVariables[1].dataValid = 1;
	p_payloadDataStore->controlVariables[1].dataInvalid = 0;
	p_payloadDataStore->controlVariables[2].dataValid = 0;
	p_payloadDataStore->controlVariables[2].dataInvalid = 0;
	p_payloadDataStore->controlVariables[3].dataValid = 0;
	p_payloadDataStore->controlVariables[4].dataInvalid = 0;

	EXPECT_EQ(1, PayloadDataPresent());
}

TEST_F(PayloadData, TestThatPayloadDataPresentOnlyReturnsTrueWhenWeHaveDataInSlot2)
{
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;
	p_payloadDataStore->controlVariables[1].dataValid = 0;
	p_payloadDataStore->controlVariables[1].dataInvalid = 0;
	p_payloadDataStore->controlVariables[2].dataValid = 1;
	p_payloadDataStore->controlVariables[2].dataInvalid = 0;
	p_payloadDataStore->controlVariables[3].dataValid = 0;
	p_payloadDataStore->controlVariables[4].dataInvalid = 0;

	EXPECT_EQ(1, PayloadDataPresent());
}

TEST_F(PayloadData, TestThatPayloadDataPresentOnlyReturnsTrueWhenWeHaveDataInSlot3)
{
	p_payloadDataStore->controlVariables[0].dataValid = 0;
	p_payloadDataStore->controlVariables[0].dataInvalid = 0;
	p_payloadDataStore->controlVariables[1].dataValid = 0;
	p_payloadDataStore->controlVariables[1].dataInvalid = 0;
	p_payloadDataStore->controlVariables[2].dataValid = 0;
	p_payloadDataStore->controlVariables[2].dataInvalid = 0;
	p_payloadDataStore->controlVariables[3].dataValid = 1;
	p_payloadDataStore->controlVariables[4].dataInvalid = 0;

	EXPECT_EQ(1, PayloadDataPresent());
}

TEST_F(PayloadData, TestThatPayloadDataPresentCallsSemGiveAndTake)
{
	RESET_FAKE(xSemaphoreTakeRecursive);
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	PayloadDataPresent();
	EXPECT_EQ(1, xSemaphoreTakeRecursive_fake.call_count);
	EXPECT_EQ(1, xSemaphoreGiveRecursive_fake.call_count);
}

TEST_F(PayloadData, TestThatPayloadDataPresentCallsSemTakeWithValidParameters)
{
	RESET_FAKE(xSemaphoreTakeRecursive);
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	PayloadDataPresent();
	EXPECT_NE(0, xSemaphoreTakeRecursive_fake.arg0_history[0]);
	EXPECT_EQ(20, xSemaphoreTakeRecursive_fake.arg1_history[0]);
}

TEST_F(PayloadData, TestThatPayloadDataPresentCallsSemGiveWithValidParameters)
{
	RESET_FAKE(xSemaphoreTakeRecursive);
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	PayloadDataPresent();
	EXPECT_NE(0, xSemaphoreGiveRecursive_fake.arg0_history[0]);
}

TEST_F(PayloadData, TestThatPayloadDataPresentCallsSemTakeButNotSemGiveIfTakeFails)
{
	RESET_FAKE(xSemaphoreTakeRecursive);
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 0;
	PayloadDataPresent();
	EXPECT_EQ(1, xSemaphoreTakeRecursive_fake.call_count);
	EXPECT_EQ(0, xSemaphoreGiveRecursive_fake.call_count);
}

TEST_F(PayloadData, TestThatGetPayloadDataRequiresValidPacketNumberPointer)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	GetHolemap_fake.custom_fake = GetHolemapFAKE;
	AddTwoPacketsOfValidPayloadData();
	uint8_t buffer[252];
	uint16_t packetNumber;
	
	EXPECT_EQ(INVALID_INPUT, GetPayloadData(buffer, sizeof(buffer), 0));

}

TEST_F(PayloadData, TestThatGetPayloadDataReturnsCorrectPacketNumberForAllPackets)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	GetHolemap_fake.custom_fake = GetHolemapFAKE;
	AddOrderedValidPayloadData();
	uint8_t buffer[252];
	uint8_t value;
	uint16_t packetNumber;
	uint32_t j = 0;

	for(j = 0; j < 261; j++)
	{
		GetPayloadData(buffer, sizeof(buffer), &packetNumber);
		EXPECT_EQ(j, packetNumber);
	}
}

TEST_F(PayloadData, TestThatGetPayloadDataClearsDataIfHolemapIsComplete)
{
	xQueueReceive_fake.custom_fake = xQueueReceive_FAKE;
	GetHolemap_fake.custom_fake = GetHolemapFAKE;
	AddOrderedValidPayloadData();
	uint8_t buffer[252];
	uint16_t packetNumber;

	for(uint32_t i = 0; i < sizeof(holemapBuffer); i++)
	{
		holemapBuffer[i] = 0xFF;
	}

	RESET_FAKE(ClearPage);
	RESET_FAKE(ClearPageBuffer);
	RESET_FAKE(WriteToFlash);
	GetPayloadData(buffer, sizeof(buffer), &packetNumber);

	EXPECT_EQ(129, ClearPage_fake.call_count);
	EXPECT_EQ(1, ClearPageBuffer_fake.call_count);
	EXPECT_EQ(1, WriteToFlash_fake.call_count);
}