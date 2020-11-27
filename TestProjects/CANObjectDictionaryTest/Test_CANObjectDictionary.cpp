#include "gtest/gtest.h"

extern "C"
{
	#include "fff.h"
	#include "CANObjectDictionary.h"
	#include "CANObjectDictionary.c"
	#include "CANODInterface.h"
	#include "Fake_CAN_PDO.h"
	#include "Fake_semphr.h"
}

static uint32_t ChangeEndianismHelper(uint32_t z_input);

DEFINE_FFF_GLOBALS;
#define CAN_OD_LENGTH 0x63

class CANObjectDictionary : public testing::Test
{
public:

	void SetUp()
	{
		RESET_FAKE(ReceivedPayloadDataTransferProtocolRequest);
		RESET_FAKE(xSemaphoreCreateRecursiveMutexStatic);
		RESET_FAKE(xSemaphoreGiveRecursive);
		RESET_FAKE(xSemaphoreTakeRecursive);

		for(uint32_t i = 0; i <= CAN_OD_LENGTH; i++)
		{
			objectDictionary[i].changed = ENTRY_UNTOUCHED;
			objectDictionary[i].data[0] = 0;
			objectDictionary[i].data[1] = 0;
			objectDictionary[i].data[2] = 0;
		}

		xSemaphoreCreateRecursiveMutexStatic_fake.return_val = 0xDEADBEEF;
		InitialiseCanObjectDictionary();
		xSemaphoreTakeRecursive_fake.return_val = 1;

		FFF_RESET_HISTORY();
	}

	virtual void TearDown() 
	{

	}
};

/***********************************************************/
/* Test OD Initialise									   */
/***********************************************************/
TEST_F(CANObjectDictionary, CANObjectDictionary_InitialiseCallsSemaphoreCreate)
{
	EXPECT_EQ(0x01, xSemaphoreCreateRecursiveMutexStatic_fake.call_count);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_InitialiseCallsSemaphoreCreateWithValidInput)
{
	EXPECT_NE(0x0, xSemaphoreCreateRecursiveMutexStatic_fake.arg0_val);
}


TEST_F(CANObjectDictionary, CANObjectDictionary_InitialiseDoesNotCallSemaphoreGiveIfUnsuccessfulAtAcquiringHandle)
{
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreCreateRecursiveMutexStatic_fake.return_val = 0x0;
	InitialiseCanObjectDictionary();
	EXPECT_EQ(0x0, xSemaphoreGiveRecursive_fake.call_count);
}

/***********************************************************/
/* Test Semaphore locking (CANObjectDictionarySet)		   */
/***********************************************************/
TEST_F(CANObjectDictionary, CANObjectDictionarySet_ReturnsErrorIfSemaphoreInvalid)
{
	xSemaphoreCreateRecursiveMutexStatic_fake.return_val = 0x0;
	InitialiseCanObjectDictionary();
	xSemaphoreTakeRecursive_fake.return_val = 1;
	EXPECT_EQ(SEMAPHORE_INVALID, SetCANObjectData(0x10, 0x01));
}

TEST_F(CANObjectDictionary, CANObjectDictionarySet_ReturnsErrorIfSemaphoreTaken)
{
	xSemaphoreTakeRecursive_fake.return_val = 0;	
	EXPECT_EQ(SEMAPHORE_TIME_OUT, SetCANObjectData(0x10, 0x02));
}

TEST_F(CANObjectDictionary, CANObjectDictionarySet_CannotSetValueIfSemaphoreTaken)
{
	uint32_t data;
	xSemaphoreTakeRecursive_fake.return_val = 1;
	SetCANObjectData(0x10, 0x01);
	xSemaphoreTakeRecursive_fake.return_val = 0;
	SetCANObjectData(0x10, 0x02);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	GetCANObjectData(0x10, &data);

	EXPECT_EQ(0x1, data);
}

TEST_F(CANObjectDictionary, CANObjectDictionarySet_SemaphorePassedCorrectHandle)
{
	RESET_FAKE(xSemaphoreTakeRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	SetCANObjectData(0x10, 0x01);
	EXPECT_EQ(0xDEADBEEF, xSemaphoreTakeRecursive_fake.arg0_val);
}

TEST_F(CANObjectDictionary, CANObjectDictionarySet_SemaphorePassedCorrectNumberOfTicks)
{
	RESET_FAKE(xSemaphoreTakeRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	SetCANObjectData(0x10, 0x01);
	EXPECT_EQ(5, xSemaphoreTakeRecursive_fake.arg1_val);
}

TEST_F(CANObjectDictionary, CANObjectDictionarySet_SemaphoreReturnedCorrectly)
{
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	SetCANObjectData(0x10, 0x01);
	EXPECT_EQ(1, xSemaphoreGiveRecursive_fake.call_count);
}

TEST_F(CANObjectDictionary, CANObjectDictionarySet_SemaphoreNotReturnedIfGiveNotSucessfulFirst)
{
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 0;
	SetCANObjectData(0x10, 0x01);
	EXPECT_EQ(0, xSemaphoreGiveRecursive_fake.call_count);
}

/***********************************************************/
/* Test Semaphore locking (CANObjectDictionaryGet)		   */
/***********************************************************/
TEST_F(CANObjectDictionary, CANObjectDictionaryGet_ReturnsErrorIfSemaphoreInvalid)
{
	uint32_t data;
	xSemaphoreCreateRecursiveMutexStatic_fake.return_val = 0x0;
	InitialiseCanObjectDictionary();
	xSemaphoreTakeRecursive_fake.return_val = 1;
	EXPECT_EQ(SEMAPHORE_INVALID, GetCANObjectData(0x10, &data));
}

TEST_F(CANObjectDictionary, CANObjectDictionaryGet_ReturnsErrorIfSemaphoreTaken)
{
	uint32_t data;
	xSemaphoreTakeRecursive_fake.return_val = 0;	
	EXPECT_EQ(SEMAPHORE_TIME_OUT, GetCANObjectData(0x10, &data));
}

TEST_F(CANObjectDictionary, CANObjectDictionaryGet_CannotGetValueIfSemaphoreTaken)
{
	uint32_t data = 4;
	xSemaphoreTakeRecursive_fake.return_val = 1;
	SetCANObjectData(0x10, 0x01);
	xSemaphoreTakeRecursive_fake.return_val = 0;
	GetCANObjectData(0x10, &data);

	EXPECT_EQ(0x0, data);
}

TEST_F(CANObjectDictionary, CANObjectDictionaryGet_SemaphorePassedCorrectHandle)
{
	uint32_t data;
	RESET_FAKE(xSemaphoreTakeRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	GetCANObjectData(0x10, &data);
	EXPECT_EQ(0xDEADBEEF, xSemaphoreTakeRecursive_fake.arg0_val);
}

TEST_F(CANObjectDictionary, CANObjectDictionaryGet_SemaphorePassedCorrectNumberOfTicks)
{
	uint32_t data;
	RESET_FAKE(xSemaphoreTakeRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	GetCANObjectData(0x10, &data);
	EXPECT_EQ(5, xSemaphoreTakeRecursive_fake.arg1_val);
}

TEST_F(CANObjectDictionary, CANObjectDictionaryGet_SemaphoreGiveReturnsCorrectly)
{
	uint32_t data;
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	GetCANObjectData(0x10, &data);
	EXPECT_EQ(1, xSemaphoreGiveRecursive_fake.call_count);
}

TEST_F(CANObjectDictionary, CANObjectDictionaryGet_SemaphoreNotReturnedIfGiveNotSucessfulFirst)
{
	uint32_t data;
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 0;
	GetCANObjectData(0x10, &data);
	EXPECT_EQ(0, xSemaphoreGiveRecursive_fake.call_count);
}

/***********************************************************/
/* Test Semaphore locking (CANObjectDictionarySet)		   */
/***********************************************************/
TEST_F(CANObjectDictionary, CANObjectDictionarySetRaw_ReturnsErrorIfSemaphoreInvalid)
{
	xSemaphoreCreateRecursiveMutexStatic_fake.return_val = 0x0;
	InitialiseCanObjectDictionary();
	xSemaphoreTakeRecursive_fake.return_val = 1;
	EXPECT_EQ(SEMAPHORE_INVALID, SetCANObjectDataRaw(0x10, 0x01));
}

TEST_F(CANObjectDictionary, CANObjectDictionarySetRaw_ReturnsErrorIfSemaphoreTaken)
{
	xSemaphoreTakeRecursive_fake.return_val = 0;	
	EXPECT_EQ(SEMAPHORE_TIME_OUT, SetCANObjectDataRaw(0x10, 0x02));
}

TEST_F(CANObjectDictionary, CANObjectDictionarySetRaw_CannotSetValueIfSemaphoreTaken)
{
	uint32_t data;
	xSemaphoreTakeRecursive_fake.return_val = 1;
	SetCANObjectDataRaw(0x10, 0x01);
	xSemaphoreTakeRecursive_fake.return_val = 0;
	SetCANObjectDataRaw(0x10, 0x02);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	GetCANObjectDataRaw(0x10, &data);

	EXPECT_EQ(0x1, data);
}

TEST_F(CANObjectDictionary, CANObjectDictionarySetRaw_SemaphorePassedCorrectHandle)
{
	RESET_FAKE(xSemaphoreTakeRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	SetCANObjectDataRaw(0x10, 0x01);
	EXPECT_EQ(0xDEADBEEF, xSemaphoreTakeRecursive_fake.arg0_val);
}

TEST_F(CANObjectDictionary, CANObjectDictionarySetRaw_SemaphorePassedCorrectNumberOfTicks)
{
	RESET_FAKE(xSemaphoreTakeRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	SetCANObjectDataRaw(0x10, 0x01);
	EXPECT_EQ(5, xSemaphoreTakeRecursive_fake.arg1_val);
}

TEST_F(CANObjectDictionary, CANObjectDictionarySetRaw_SemaphoreReturnedCorrectly)
{
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	SetCANObjectDataRaw(0x10, 0x01);
	EXPECT_EQ(1, xSemaphoreGiveRecursive_fake.call_count);
}

TEST_F(CANObjectDictionary, CANObjectDictionarySetRaw_SemaphoreNotReturnedIfGiveNotSucessfulFirst)
{
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 0;
	SetCANObjectDataRaw(0x10, 0x01);
	EXPECT_EQ(0, xSemaphoreGiveRecursive_fake.call_count);
}

/***********************************************************/
/* Test Semaphore locking (CANObjectDictionaryGetRaw)		   */
/***********************************************************/
TEST_F(CANObjectDictionary, CANObjectDictionaryGetRaw_ReturnsErrorIfSemaphoreInvalid)
{
	uint32_t data;
	xSemaphoreCreateRecursiveMutexStatic_fake.return_val = 0x0;
	InitialiseCanObjectDictionary();
	xSemaphoreTakeRecursive_fake.return_val = 1;
	EXPECT_EQ(SEMAPHORE_INVALID, GetCANObjectDataRaw(0x10, &data));
}

TEST_F(CANObjectDictionary, CANObjectDictionaryGetRaw_ReturnsErrorIfSemaphoreTaken)
{
	uint32_t data;
	xSemaphoreTakeRecursive_fake.return_val = 0;	
	EXPECT_EQ(SEMAPHORE_TIME_OUT, GetCANObjectDataRaw(0x10, &data));
}

TEST_F(CANObjectDictionary, CANObjectDictionaryGetRaw_CannotGetValueIfSemaphoreTaken)
{
	uint32_t data = 4;
	xSemaphoreTakeRecursive_fake.return_val = 1;
	SetCANObjectDataRaw(0x10, 0x01);
	xSemaphoreTakeRecursive_fake.return_val = 0;
	GetCANObjectDataRaw(0x10, &data);

	EXPECT_EQ(0x0, data);
}

TEST_F(CANObjectDictionary, CANObjectDictionaryGetRaw_SemaphorePassedCorrectHandle)
{
	uint32_t data;
	RESET_FAKE(xSemaphoreTakeRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	GetCANObjectDataRaw(0x10, &data);
	EXPECT_EQ(0xDEADBEEF, xSemaphoreTakeRecursive_fake.arg0_val);
}

TEST_F(CANObjectDictionary, CANObjectDictionaryGetRaw_SemaphorePassedCorrectNumberOfTicks)
{
	uint32_t data;
	RESET_FAKE(xSemaphoreTakeRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	GetCANObjectDataRaw(0x10, &data);
	EXPECT_EQ(5, xSemaphoreTakeRecursive_fake.arg1_val);
}

TEST_F(CANObjectDictionary, CANObjectDictionaryGetRaw_SemaphoreGiveReturnsCorrectly)
{
	uint32_t data;
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 1;
	GetCANObjectDataRaw(0x10, &data);
	EXPECT_EQ(1, xSemaphoreGiveRecursive_fake.call_count);
}

TEST_F(CANObjectDictionary, CANObjectDictionaryGetRaw_SemaphoreNotReturnedIfGiveNotSucessfulFirst)
{
	uint32_t data;
	RESET_FAKE(xSemaphoreGiveRecursive);
	xSemaphoreTakeRecursive_fake.return_val = 0;
	GetCANObjectDataRaw(0x10, &data);
	EXPECT_EQ(0, xSemaphoreGiveRecursive_fake.call_count);
}

/***********************************************************/
/* Test SetCANObject    								   */
/***********************************************************/
TEST_F(CANObjectDictionary, CANObjectDictionary_SetCANObjectDataReturnsErrorWithInvalidAccessRights)
{
	uint32_t data = 0;
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x0, data));						 //0x0
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x1, data));						 //0x1
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x2, data));						 //0x2
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			SetCANObjectData(0x3, data));						 //0x3
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			SetCANObjectData(0x4, data));						 //0x4
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			SetCANObjectData(0x5, data));						 //0x5
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			SetCANObjectData(0x6, data));						 //0x6
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			SetCANObjectData(0x7, data));						 //0x7
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			SetCANObjectData(0x8, data));						 //0x8
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x9, data));						 //0x9
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0xA, data));						 //0xA
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			SetCANObjectData(0xB, data));						 //0xB
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0xC, data));						 //0xC
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0xD, data));						 //0xD
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0xE, data));						 //0xE
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0xF, data));						 //0xF
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x10, data));						 //0x10
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x11, data));						 //0x11
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x12, data));						 //0x12
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x13, data));						 //0x13
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x14, data));						 //0x14
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x15, data));						 //0x15
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x16, data));						 //0x16
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x17, data));						 //0x17
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x18, data));						 //0x18
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x19, data));						 //0x19
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x1A, data));						 //0x1A
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x1B, data));						 //0x1B
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x1C, data));						 //0x1C
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x1D, data));						 //0x1D
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x1E, data));						 //0x1E
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x1F, data));						 //0x1F
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x20, data));						 //0x20
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x21, data));						 //0x21
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x22, data));						 //0x22
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x23, data));						 //0x23
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x24, data));						 //0x24
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x25, data));						 //0x25
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x26, data));						 //0x26
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x27, data));						 //0x27
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x28, data));						 //0x28
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x29, data));						 //0x29
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x2A, data));						 //0x2A
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x2B, data));						 //0x2B
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x2C, data));						 //0x2C
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x2D, data));						 //0x2D
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x2E, data));						 //0x2E
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x2F, data));						 //0x2F
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x30, data));						 //0x30
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x31, data));						 //0x31
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x32, data));						 //0x32
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x33, data));						 //0x33
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x34, data));						 //0x34
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x35, data));						 //0x35
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x36, data));						 //0x36
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x37, data));						 //0x37
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x38, data));						 //0x38
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x39, data));						 //0x39
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x3A, data));						 //0x3A
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x3B, data));						 //0x3B
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x3C, data));						 //0x3C
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x3D, data));						 //0x3D
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x3E, data));						 //0x3E
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x3F, data));						 //0x3F
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x40, data));						 //0x40
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x41, data));						 //0x41
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x42, data));						 //0x42
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x43, data));						 //0x43
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x44, data));						 //0x44
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x45, data));						 //0x45
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						SetCANObjectData(0x46, data));						 //0x46
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x47, data));						 //0x47
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x48, data));						 //0x48
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x49, data));						 //0x49
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x4A, data));						 //0x4A
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x4B, data));						 //0x4B
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x4C, data));						 //0x4C
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x4D, data));						 //0x4D
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x4E, data));						 //0x4E
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x4F, data));						 //0x4F
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x50, data));						 //0x50
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x51, data));						 //0x51
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x52, data));						 //0x52
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x53, data));						 //0x53
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x54, data));						 //0x54
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x55, data));						 //0x55
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x56, data));						 //0x56
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x57, data));						 //0x57
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x58, data));						 //0x58
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x59, data));						 //0x59
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x5A, data));						 //0x5A
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x5B, data));						 //0x5B
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x5C, data));						 //0x5C
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x5D, data));						 //0x5D
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x5E, data));						 //0x5E
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x5F, data));						 //0x5F
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x60, data));						 //0x60
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x61, data));						 //0x61
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				SetCANObjectData(0x62, data));						 //0x62
}

TEST_F(CANObjectDictionary, CANObjectDictionary_SetCANObjectDataSetsTheCorrectValuesInTheObjectDictionary)
{
	for(uint32_t i = 0; i < CAN_OD_LENGTH; i++)
	{
		objectDictionary[i].data[0] = 0xFFF;
		objectDictionary[i].data[1] = 0xFFF;
		objectDictionary[i].data[2] = 0xFFF;

		if(i != 0x17)		//FM slot field is a special case.
		{
			if(SetCANObjectData(i, i) == NO_ERROR_OBJECT_DICTIONARY)
			{
				EXPECT_EQ(ChangeEndianismHelper(i), objectDictionary[i].data[0]);
			}
			else
			{
				EXPECT_EQ(0xFFF, objectDictionary[i].data[0]);
			}
		}
	}
}

/***********************************************************/
/* Test GetCANObject    								   */
/***********************************************************/
TEST_F(CANObjectDictionary, CANObjectDictionary_GetCANObjectDataReturnsErrorWithInvalidIndex)
{
	uint32_t data;
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x0, &data));					 //0x0
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x1, &data));					 //0x1
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x2, &data));					 //0x2
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			GetCANObjectData(0x3, &data));					 //0x3
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			GetCANObjectData(0x4, &data));					 //0x4
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			GetCANObjectData(0x5, &data));					 //0x5
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			GetCANObjectData(0x6, &data));					 //0x6
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			GetCANObjectData(0x7, &data));					 //0x7
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			GetCANObjectData(0x8, &data));					 //0x8
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x9, &data));					 //0x9
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0xA, &data));					 //0xA
	EXPECT_EQ(INVALID_INDEX_OBJECT_DICTIONARY,			GetCANObjectData(0xB, &data));					 //0xB
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0xC, &data));					 //0xC
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0xD, &data));					 //0xD
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0xE, &data));					 //0xE
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0xF, &data));					 //0xF
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x10, &data));					 //0x10
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x11, &data));					 //0x11
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x12, &data));					 //0x12
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x13, &data));					 //0x13
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x14, &data));					 //0x14
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x15, &data));					 //0x15
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x16, &data));					 //0x16
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x17, &data));					 //0x17
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x18, &data));					 //0x18
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x19, &data));					 //0x19
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x1A, &data));					 //0x1A
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x1B, &data));					 //0x1B
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x1C, &data));					 //0x1C
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x1D, &data));					 //0x1D
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x1E, &data));					 //0x1E
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x1F, &data));					 //0x1F
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x20, &data));					 //0x20
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x21, &data));					 //0x21
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x22, &data));					 //0x22
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x23, &data));					 //0x23
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x24, &data));					 //0x24
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x25, &data));					 //0x25
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x26, &data));					 //0x26
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x27, &data));					 //0x27
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x28, &data));					 //0x28
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x29, &data));					 //0x29
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x2A, &data));					 //0x2A
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x2B, &data));					 //0x2B
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x2C, &data));					 //0x2C
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x2D, &data));					 //0x2D
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x2E, &data));					 //0x2E
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x2F, &data));					 //0x2F
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x30, &data));					 //0x30
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x31, &data));					 //0x31
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x32, &data));					 //0x32
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x33, &data));					 //0x33
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x34, &data));					 //0x34
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x35, &data));					 //0x35
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x36, &data));					 //0x36
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x37, &data));					 //0x37
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x38, &data));					 //0x38
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x39, &data));					 //0x39
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x3A, &data));					 //0x3A
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x3B, &data));					 //0x3B
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x3C, &data));					 //0x3C
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x3D, &data));					 //0x3D
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x3E, &data));					 //0x3E
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x3F, &data));					 //0x3F
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x40, &data));					 //0x40
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x41, &data));					 //0x41
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x42, &data));					 //0x42
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x43, &data));					 //0x43
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x44, &data));					 //0x44
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x45, &data));					 //0x45
	EXPECT_EQ(NO_ERROR_OBJECT_DICTIONARY,				GetCANObjectData(0x46, &data));					 //0x46
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x47, &data));					 //0x47
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x48, &data));					 //0x48
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x49, &data));					 //0x49
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x4A, &data));					 //0x4A
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x4B, &data));					 //0x4B
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x4C, &data));					 //0x4C
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x4D, &data));					 //0x4D
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x4E, &data));					 //0x4E
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x4F, &data));					 //0x4F
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x50, &data));					 //0x50
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x51, &data));					 //0x51
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x52, &data));					 //0x52
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x53, &data));					 //0x53
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x54, &data));					 //0x54
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x55, &data));					 //0x55
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x56, &data));					 //0x56
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x57, &data));					 //0x57
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x58, &data));					 //0x58
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x59, &data));					 //0x59
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x5A, &data));					 //0x5A
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x5B, &data));					 //0x5B
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x5C, &data));					 //0x5C
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x5D, &data));					 //0x5D
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x5E, &data));					 //0x5E
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x5F, &data));					 //0x5F
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x60, &data));					 //0x60
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x61, &data));					 //0x61
	EXPECT_EQ(WRONG_ACCESS_RIGHTS,						GetCANObjectData(0x62, &data));					 //0x62
}

TEST_F(CANObjectDictionary, CANObjectDictionary_GetCANObjectDataReturnsErrorForNULLPointer)
{
	EXPECT_EQ(NULL_POINTER_OBJECT_DICTIONARY, GetCANObjectData(120, (uint32_t*)0));
}

TEST_F(CANObjectDictionary, CANObjectDictionary_GetCANObjectDataReturnsCorrectValues)
{
	uint32_t data;

	for(uint32_t i = 0; i < CAN_OD_LENGTH; i++)
	{
		objectDictionary[i].data[0] = i;
		objectDictionary[i].data[1] = i;
		objectDictionary[i].data[2] = i;
		if(i != 0x16 && i != 0x19)		//We dont want to test the fitter / holemap fields here.
		{
			if(GetCANObjectData(i, &data) == NO_ERROR_OBJECT_DICTIONARY)
			{
				EXPECT_EQ(ChangeEndianismHelper(i), data);
			}
			else
			{
				EXPECT_EQ(0, data);
			}
		}
	}
}

/***********************************************************/
/* Test perform actions  								   */
/***********************************************************/
TEST_F(CANObjectDictionary, PerformEventActions_CallsReceivedPayloadDataTransferProtocolRequest)
{
	SetCANObjectData(0x14, 0xAA);
	//PerformEventActions(0x14);
	EXPECT_EQ(1, ReceivedPayloadDataTransferProtocolRequest_fake.call_count);
}

TEST_F(CANObjectDictionary, PerformEventActions_CallsReceivedPayloadDataTransferProtocolRequestWithCorrectFlagWritten)
{
	SetCANObjectData(0x14, 0xAA);
	SetCANObjectData(0x14, 0xAA);
	EXPECT_EQ(0, ReceivedPayloadDataTransferProtocolRequest_fake.arg2_val);
}

TEST_F(CANObjectDictionary, PerformEventActions_CallsReceivedPayloadDataTransferProtocolRequestWithCorrectFlagChanged)
{
	SetCANObjectData(0x14, 0xAA);
	objectDictionary[0x14].changed = ENTRY_CHANGED;
	EXPECT_EQ(1, ReceivedPayloadDataTransferProtocolRequest_fake.arg2_val);
}

TEST_F(CANObjectDictionary, PerformEventActions_ResetsTheEventStateIfActionMethodReturnsNoError)
{
	ReceivedPayloadDataTransferProtocolRequest_fake.return_val = NO_ERROR_CAN_OPEN;
	objectDictionary[0x14].changed = ENTRY_CHANGED;
	SetCANObjectData(0x14, 0xAA);
	EXPECT_EQ(ENTRY_UNTOUCHED, objectDictionary[0x14].changed);
}

TEST_F(CANObjectDictionary, PerformEventActions_DoesntResetsTheEventStateIfActionMethodReturnsError)
{
	ReceivedPayloadDataTransferProtocolRequest_fake.return_val = COULD_NOT_EXECUTE_ACTION;
	objectDictionary[0x14].changed = ENTRY_CHANGED;
	SetCANObjectData(0x14, 0xAA);
	EXPECT_EQ(ENTRY_CHANGED, objectDictionary[0x14].changed);
}


static uint32_t ChangeEndianismHelper(uint32_t z_input)
{
		ENDIANISM_FLIP flip;
		uint8_t temp;
		flip.data = z_input;
		temp = flip.data0;
		flip.data0 = flip.data3;
		flip.data3 = temp;
		temp = flip.data1;
		flip.data1 = flip.data2;
		flip.data2 = temp;
		return flip.data;
}

/***********************************************************/
/* Test SettCANObject (time)							   */
/***********************************************************/
TEST_F(CANObjectDictionary, CANObjectDictionary_SettCANObjectDataSetsTheCorrectValuesInTheObjectDictionaryForDate)
{
	SettCANObjectData(0x01, 0xAB);
	EXPECT_EQ(ChangeEndianismHelper(0xAB), objectDictionary[0x01].data[0]);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_SettCANObjectDataSetsTheCorrectValuesInTheObjectDictionaryForTime)
{
	SettCANObjectData(0x02, 0xAC);
	EXPECT_EQ(ChangeEndianismHelper(0xAC), objectDictionary[0x02].data[0]);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_SettCANObjectDataDoesNotSetValuesInTheObjectDictionaryOutsideSETTPermissions)
{
	SettCANObjectData(0x03, 0xBB);
	EXPECT_NE(ChangeEndianismHelper(0xBB), objectDictionary[0x03].data[0]);
}


TEST_F(CANObjectDictionary, CANObjectDictionary_SettCANObjectDataSetsTheChangedFlagCorrectly)
{
	SettCANObjectData(0x01, 0xAA);
	EXPECT_EQ(1, objectDictionary[0x01].changed);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_SettCANObjectDataWriteOfSameDataDoesntOverwriteTheChangedFlag)
{
	SettCANObjectData(0x01, 0xAA);
	SettCANObjectData(0x01, 0xAA);
	EXPECT_EQ(1, objectDictionary[0x01].changed);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_SettCANObjectDataSetsTheWrittenFlagCorrectly)
{
	objectDictionary[0x01].data[0] = 0xAA000000;
	objectDictionary[0x01].data[1] = 0xAA000000;
	objectDictionary[0x01].data[2] = 0xAA000000;

	SettCANObjectData(0x01, 0xAA);
	EXPECT_EQ(2, objectDictionary[0x01].changed);
}

TEST_F(CANObjectDictionary, GetFieldTypeReturnCorrectlyForNormalFields)
{
	for(uint16_t i = 0; i < CAN_OD_LENGTH; i++)
	{
		if(i != 0x16 && i != 0x19 && i != 0x21)
		{
			EXPECT_EQ(NORMAL_FIELD, GetFieldType(i));
		}
	}
}

TEST_F(CANObjectDictionary, GetFieldTypeReturnCorrectlyForFitterMessages)
{
	EXPECT_EQ(FITTER_FIELD, GetFieldType(AMS_OBC_FM_REGISTER));
}

TEST_F(CANObjectDictionary, GetFieldTypeReturnCorrectlyForHolemaps)
{
	EXPECT_EQ(HOLEMAP_FIELD, GetFieldType(AMS_OBC_HM_REGISTER));
}

TEST_F(CANObjectDictionary, GetFieldTypeReturnCorrectlyForDebug)
{
	EXPECT_EQ(DEBUG_FIELD, GetFieldType(AMS_OBC_DEBUG_REGISTER));
}

/***********************************************************/
/* Test holemap fields.									   */
/***********************************************************/
TEST_F(CANObjectDictionary, CANObjectDictionary_TestHolemapReturnsCorrectValue)
{
	uint32_t data;

	SetCANObjectData(0x16, 0x10);
	SetCANObjectData(0x15, 0x0);
	GetCANObjectData(0x16, &data);

	EXPECT_EQ(0x10, data);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestHolemapSetIncrementsInputCounter)
{
	uint32_t data;
	SetCANObjectData(0x15, 0x0);		// Holemap index
	SetCANObjectData(0x16, 0x10);		// Holemap data
	GetCANObjectData(0x15, &data);		

	EXPECT_EQ(ChangeEndianismHelper(0x1), data);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestHolemapCanAddValuesAndThenGetThemCorrectly)
{
	uint32_t data;

	SetCANObjectData(0x15, 0x0);
	for(uint32_t i = 0; i < 64; i++)
	{
		SetCANObjectData(0x16, i);
	}
	SetCANObjectData(0x15, 0x0);
	for(uint32_t i = 0; i < 64; i++)
	{
		GetCANObjectData(0x16, &data);
		EXPECT_EQ(i, data);
	}
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestHolemapLoopsAtArrayBoundary)
{
	uint32_t data;

	SetCANObjectData(0x15, 60);
	for(uint32_t i = 0; i < 20; i++)
	{
		SetCANObjectData(0x16, i);
	}
	SetCANObjectData(0x15, 60);
	for(uint32_t i = 0; i < 20; i++)
	{
		GetCANObjectData(0x16, &data);
		EXPECT_EQ(i, data);
	}
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestHolemapCanAddValuesAndThenGetThemCorrectlyWithGetMethod)
{
	uint32_t buffer[64];

	SetCANObjectData(0x15, 0x0);
	for(uint32_t i = 0; i < 64; i++)
	{
		SetCANObjectData(0x16, i);
	}

	GetHolemap((uint8_t*)buffer, sizeof(buffer));

	for(uint32_t i = 0; i < 64; i++)
	{
		EXPECT_EQ(i, ChangeEndianismHelper(buffer[i]));
	}
}

/***********************************************************/
/* Test fitter messages									   */
/***********************************************************/
TEST_F(CANObjectDictionary, CANObjectDictionary_TestFitterMessageReturnsCorrectValue)
{
	uint32_t data;

	SetCANObjectData(0x19, 0x10);
	SetCANObjectData(0x18, 0x0);
	GetCANObjectData(0x19, &data);

	EXPECT_EQ(0x10, data);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestFitterMessageSetIncrementsInputCounter)
{
	uint32_t data;
	SetCANObjectData(0x18, 0x0);		// Holemap index
	SetCANObjectData(0x19, 0x10);		// Holemap data
	GetCANObjectData(0x18, &data);		

	EXPECT_EQ(ChangeEndianismHelper(0x1), data);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestFitterMessageCanAddValuesAndThenGetThemCorrectly)
{
	uint32_t data;

	SetCANObjectData(0x18, 0x0);
	for(uint32_t i = 0; i < 50; i++)
	{
		SetCANObjectData(0x19, i);
	}
	SetCANObjectData(0x18, 0x0);
	for(uint32_t i = 0; i < 50; i++)
	{
		GetCANObjectData(0x19, &data);
		EXPECT_EQ(i, data);
	}
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestFitterMessageLoopsAtArrayBoundary)
{
	uint32_t data;

	SetCANObjectData(0x18, 60);
	for(uint32_t i = 0; i < 20; i++)
	{
		SetCANObjectData(0x19, i);
	}
	SetCANObjectData(0x18, 60);
	for(uint32_t i = 0; i < 20; i++)
	{
		GetCANObjectData(0x19, &data);
		EXPECT_EQ(i, data);
	}
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestFitterMessageSetIncrementsInputCounterOnSlotBoundary)
{
	uint32_t data;
	SetCANObjectData(0x17, 0x2);		// Fitter message slot
	SetCANObjectData(0x18, 0x0);		// Holemap index
	SetCANObjectData(0x19, 0x10);		// Holemap data
	GetCANObjectData(0x18, &data);		

	EXPECT_EQ(ChangeEndianismHelper(0x1), data);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestFitterMessageSlotFieldLimitedToMaximumNumberOfSlots)
{
	uint32_t data;
	SetCANObjectData(0x17, 0x4);		// Fitter message slot
	GetCANObjectData(0x17, &data);		

	EXPECT_EQ(ChangeEndianismHelper(0x3), data);
}



TEST_F(CANObjectDictionary, CANObjectDictionary_TestFitterMessageAreAddedToCOrrectSlot)
{
	uint32_t data;

	SetCANObjectData(0x17, 0x0);
	for(uint32_t i = 0; i < 10; i++)
	{
		SetCANObjectData(0x19, i);
	}

	SetCANObjectData(0x17, ChangeEndianismHelper(0x00000001));
	for(uint32_t i = 10; i < 20; i++)
	{
		SetCANObjectData(0x19, i);
	}

	SetCANObjectData(0x17, ChangeEndianismHelper(0x2));
	for(uint32_t i = 20; i < 30; i++)
	{
		SetCANObjectData(0x19, i);
	}


	SetCANObjectData(0x17, 0x0);
	for(uint32_t i = 0; i < 10; i++)
	{
		GetCANObjectData(0x19, &data);
		EXPECT_EQ(i, data);
	}

	SetCANObjectData(0x17, ChangeEndianismHelper(0x1));
	for(uint32_t i = 10; i < 20; i++)
	{
		GetCANObjectData(0x19, &data);
		EXPECT_EQ(i, data);
	}

	SetCANObjectData(0x17, ChangeEndianismHelper(0x2));
	for(uint32_t i = 20; i < 30; i++)
	{
		GetCANObjectData(0x19, &data);
		EXPECT_EQ(i, data);
	}
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestFitterMessage0CanAddValuesAndThenGetThemCorrectlyWithGetMethod)
{
	uint32_t buffer[64];

	SetCANObjectData(0x17, ChangeEndianismHelper(0x0));
	SetCANObjectData(0x18, 0x0);
	for(uint32_t i = 0; i < 50; i++)
	{
		SetCANObjectData(0x19, i);
	}

	GetFitterMessage(0, (uint8_t*)buffer, sizeof(buffer));

	for(uint32_t i = 0; i < 50; i++)
	{
		EXPECT_EQ(i, ChangeEndianismHelper(buffer[i]));
	}
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestFitterMessage1CanAddValuesAndThenGetThemCorrectlyWithGetMethod)
{
	uint32_t buffer[64];

	SetCANObjectData(0x17, ChangeEndianismHelper(0x1));
	SetCANObjectData(0x18, 0x0);
	for(uint32_t i = 0; i < 50; i++)
	{
		SetCANObjectData(0x19, i+2);
	}

	GetFitterMessage(1, (uint8_t*)buffer, sizeof(buffer));

	for(uint32_t i = 0; i < 50; i++)
	{
		EXPECT_EQ(i+2, ChangeEndianismHelper(buffer[i]));
	}
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestFitterMessage2CanAddValuesAndThenGetThemCorrectlyWithGetMethod)
{
	uint32_t buffer[64];

	SetCANObjectData(0x17, ChangeEndianismHelper(0x2));
	SetCANObjectData(0x18, 0x0);
	for(uint32_t i = 0; i < 50; i++)
	{
		SetCANObjectData(0x19, i+4);
	}

	GetFitterMessage(2, (uint8_t*)buffer, sizeof(buffer));

	for(uint32_t i = 0; i < 50; i++)
	{
		EXPECT_EQ(i+4, ChangeEndianismHelper(buffer[i]));
	}
}

TEST_F(CANObjectDictionary, CANObjectDictionary_GetCANObjectDataRawPerformsCorrectBitMasks)
{
	uint32_t data;


	SetCANObjectDataRaw(0x0, 0xFFFFFFFF);	
	SetCANObjectDataRaw(0x1, 0xFFFFFFFF);	
	SetCANObjectDataRaw(0x2, 0xFFFFFFFF);	
	SetCANObjectDataRaw(0x3, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x4, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x5, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x6, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x7, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x8, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x9, 0xFFFFFFFF);
	SetCANObjectDataRaw(0xA, 0xFFFFFFFF);
	SetCANObjectDataRaw(0xB, 0xFFFFFFFF);
	SetCANObjectDataRaw(0xC, 0xFFFFFFFF);
	SetCANObjectDataRaw(0xD, 0xFFFFFFFF);
	SetCANObjectDataRaw(0xE, 0xFFFFFFFF);
	SetCANObjectDataRaw(0xF, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x10, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x11, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x12, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x13, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x14, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x15, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x16, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x17, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x18, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x19, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x1A, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x1B, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x1C, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x1D, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x1E, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x1F, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x20, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x21, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x22, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x23, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x24, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x25, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x26, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x27, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x28, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x29, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x2A, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x2B, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x2C, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x2D, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x2E, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x2F, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x30, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x31, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x32, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x33, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x34, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x35, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x36, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x37, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x38, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x39, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x3A, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x3B, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x3C, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x3D, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x3E, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x3F, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x40, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x41, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x42, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x43, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x44, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x45, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x46, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x47, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x48, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x49, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x4A, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x4B, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x4C, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x4D, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x4E, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x4F, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x50, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x51, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x52, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x53, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x54, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x55, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x56, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x57, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x58, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x59, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x5A, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x5B, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x5C, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x5D, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x5E, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x5F, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x60, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x61, 0xFFFFFFFF);
	SetCANObjectDataRaw(0x62, 0xFFFFFFFF);

	GetCANObjectDataRaw(0x0, &data); 
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x1, &data); 
	EXPECT_EQ(0xFFFFFF, data);
	GetCANObjectDataRaw(0x2, &data); 
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x10, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x11, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x12, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x13, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x14, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x15, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x16, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x17, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x18, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x19, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x1A, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x1B, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x1C, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x1D, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x1E, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x1F, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x20, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x21, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x22, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x23, &data);  
	EXPECT_EQ(0xFFFFFF, data);
	GetCANObjectDataRaw(0x24, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x25, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x26, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x27, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x28, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x29, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x2A, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x2B, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x2C, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x2D, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x2E, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x2F, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x30, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x31, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x32, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x33, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x34, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x35, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x36, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x37, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x38, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x39, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x3A, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x3B, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x3C, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x3D, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x3E, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x3F, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x40, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x41, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x42, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x43, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x44, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x45, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x46, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x47, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x48, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x49, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x4A, &data);  
	EXPECT_EQ(0xFF, data);
	GetCANObjectDataRaw(0x4B, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x4C, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x4D, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x4E, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x4F, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x50, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x51, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x52, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x53, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x54, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x55, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x56, &data);  
	EXPECT_EQ(0xFFFFFFFF, data);
	GetCANObjectDataRaw(0x57, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x58, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x59, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x5A, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x5B, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x5C, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x5D, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x5E, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x5F, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x60, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x61, &data);  
	EXPECT_EQ(0xFFFF, data);
	GetCANObjectDataRaw(0x62, &data);  
	EXPECT_EQ(0xFFFF, data);


}

/**********************************/
/* Extended field locking testing */
/**********************************/
TEST_F(CANObjectDictionary, CANObjectDictionary_TestHolemapSetCallsSemaphoreGiveAndTake)
{
	SetHolemapField(0, 0, 0);
	EXPECT_EQ(3, xSemaphoreGiveRecursive_fake.call_count);
	EXPECT_EQ(3, xSemaphoreTakeRecursive_fake.call_count);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestHolemapGetCallsSemaphoreGiveAndTake)
{
	uint8_t buffer[200];
	GetHolemapField(0, (uint32_t *)(&buffer[0]));
	EXPECT_EQ(3, xSemaphoreGiveRecursive_fake.call_count);
	EXPECT_EQ(3, xSemaphoreTakeRecursive_fake.call_count);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestFitterMessageFieldSetCallsSemaphoreGiveAndTake)
{
	SetFitterMessageField(0, 0, 0);
	EXPECT_EQ(4, xSemaphoreGiveRecursive_fake.call_count);
	EXPECT_EQ(4, xSemaphoreTakeRecursive_fake.call_count);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_TestFitterMessageFieldGetCallsSemaphoreGiveAndTake)
{
	uint8_t buffer[200];
	GetFitterMessageField(0, (uint32_t *)(&buffer[0]));
	EXPECT_EQ(4, xSemaphoreGiveRecursive_fake.call_count);
	EXPECT_EQ(4, xSemaphoreTakeRecursive_fake.call_count);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_ClearHoleMapCallsSemaphoreGiveAndTake)
{
	ClearHoleMap();
	EXPECT_EQ(1, xSemaphoreGiveRecursive_fake.call_count);
	EXPECT_EQ(1, xSemaphoreTakeRecursive_fake.call_count);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_GetHoleMapCallsSemaphoreGiveAndTake)
{
	uint8_t buffer[200];
	GetHolemap(buffer, sizeof(buffer));
	EXPECT_EQ(1, xSemaphoreGiveRecursive_fake.call_count);
	EXPECT_EQ(1, xSemaphoreTakeRecursive_fake.call_count);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_GetFitterMessageCallsSemaphoreGiveAndTake)
{
	uint8_t buffer[200];
	GetFitterMessage(0, buffer, sizeof(buffer));
	EXPECT_EQ(1, xSemaphoreGiveRecursive_fake.call_count);
	EXPECT_EQ(1, xSemaphoreTakeRecursive_fake.call_count);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_GetCANObjectTMRProtectionInGetCANObjectDataRaw)
{
	uint32_t data;
	objectDictionary[0x10].data[0] = 0;
	objectDictionary[0x10].data[1] = 1;
	objectDictionary[0x10].data[2] = 1;
	GetCANObjectDataRaw(0x10, &data); 
	EXPECT_EQ(1, data);

	objectDictionary[0x10].data[0] = 1;
	objectDictionary[0x10].data[1] = 0;
	objectDictionary[0x10].data[2] = 1;
	GetCANObjectDataRaw(0x10, &data); 
	EXPECT_EQ(1, data);

	objectDictionary[0x10].data[0] = 1;
	objectDictionary[0x10].data[1] = 1;
	objectDictionary[0x10].data[2] = 0;
	GetCANObjectDataRaw(0x10, &data); 
	EXPECT_EQ(1, data);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_GetCANObjectTMRProtectionInGetCANObjectData)
{
	uint32_t data;
	objectDictionary[0x10].data[0] = 0;
	objectDictionary[0x10].data[1] = 1;
	objectDictionary[0x10].data[2] = 1;
	GetCANObjectData(0x10, &data); 
	EXPECT_EQ(ChangeEndianismHelper(1), data);

	objectDictionary[0x10].data[0] = 1;
	objectDictionary[0x10].data[1] = 0;
	objectDictionary[0x10].data[2] = 1;
	GetCANObjectData(0x10, &data); 
	EXPECT_EQ(ChangeEndianismHelper(1), data);

	objectDictionary[0x10].data[0] = 1;
	objectDictionary[0x10].data[1] = 1;
	objectDictionary[0x10].data[2] = 0;
	GetCANObjectData(0x10, &data); 
	EXPECT_EQ(ChangeEndianismHelper(1), data);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_GetCANObjectTMRProtectionInSetCANObjectDataRaw)
{
	SetCANObjectDataRaw(0x10, 0x03);

	EXPECT_EQ(0x03, objectDictionary[0x10].data[0]);
	EXPECT_EQ(0x03, objectDictionary[0x10].data[1]);
	EXPECT_EQ(0x03, objectDictionary[0x10].data[2]);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_GetCANObjectTMRProtectionInSetCANObjectData)
{
	SetCANObjectData(0x10, 0x03);

	EXPECT_EQ(ChangeEndianismHelper(0x03), objectDictionary[0x10].data[0]);
	EXPECT_EQ(ChangeEndianismHelper(0x03), objectDictionary[0x10].data[1]);
	EXPECT_EQ(ChangeEndianismHelper(0x03), objectDictionary[0x10].data[2]);
}

TEST_F(CANObjectDictionary, CANObjectDictionary_GetHousekeepingDataCallsCanGetObjectCorrectNumberOfTimes)
{
	uint32_t *data;

	objectDictionary[AMS_OBC_P_UP].data[1] = 1;
	objectDictionary[AMS_OBC_P_UP].data[2] = 1;
	objectDictionary[AMS_OBC_P_UP].data[3] = 1;

	objectDictionary[AMS_OBC_P_UP_DROPPED].data[1] = 2;
	objectDictionary[AMS_OBC_P_UP_DROPPED].data[2] = 2;
	objectDictionary[AMS_OBC_P_UP_DROPPED].data[3] = 2;

	objectDictionary[AMS_OBC_MEM_STAT_1].data[1] = 3;
	objectDictionary[AMS_OBC_MEM_STAT_1].data[2] = 3;
	objectDictionary[AMS_OBC_MEM_STAT_1].data[3] = 3;

	objectDictionary[AMS_OBC_MEM_STAT_2].data[1] = 4;
	objectDictionary[AMS_OBC_MEM_STAT_2].data[2] = 4;
	objectDictionary[AMS_OBC_MEM_STAT_2].data[3] = 4;

	objectDictionary[AMS_OBC_LC].data[1] = 5;
	objectDictionary[AMS_OBC_LC].data[2] = 5;
	objectDictionary[AMS_OBC_LC].data[3] = 5;

	objectDictionary[AMS_EPS_DCDC_T].data[1] = 6;
	objectDictionary[AMS_EPS_DCDC_T].data[2] = 6;
	objectDictionary[AMS_EPS_DCDC_T].data[3] = 6;

	objectDictionary[AMS_VHF_FM_PA_T].data[1] = 7;
	objectDictionary[AMS_VHF_FM_PA_T].data[2] = 7;
	objectDictionary[AMS_VHF_FM_PA_T].data[3] = 7;

	objectDictionary[AMS_VHF_BPSK_PA_T].data[1] = 8;
	objectDictionary[AMS_VHF_BPSK_PA_T].data[2] = 8;
	objectDictionary[AMS_VHF_BPSK_PA_T].data[3] = 8;

	GetHousekeepingData((uint8_t **)&data);

	EXPECT_EQ(0x01000000, data[0]);
	EXPECT_EQ(0x02000000, data[1]);
	EXPECT_EQ(0x03000000, data[2]);
	EXPECT_EQ(0x04000000, data[3]);
	EXPECT_EQ(0x05060708, data[4]);
}
