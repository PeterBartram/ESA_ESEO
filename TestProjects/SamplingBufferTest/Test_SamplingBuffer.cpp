#include "gtest/gtest.h"

extern "C"
{
	#include "fff.h"
	#include "SamplingBuffer.h"
}

DEFINE_FFF_GLOBALS;


class SamplingBuffer : public testing::Test
{
public:
	void SetUp()
	{
		InitialiseSamplingBuffer();
	}

	virtual void TearDown() 
	{
	}
};


/***************************************************/
/* General initialisation testing.				   */
/***************************************************/
TEST_F(SamplingBuffer, TestInitialisedBufferReturnsZeroSize)
{
	EXPECT_EQ(0, SizeSamplingBuff());
}

TEST_F(SamplingBuffer, TestEmptyBufferReturnsZeroSize)
{
	float data = 0;
	EXPECT_EQ(0, UplinkInputBufferGet(&data));
}

TEST_F(SamplingBuffer, TestSetFunctionAddsOneToTheBuffer)
{
	uint16_t data = 0;
	UplinkInputBufferSet(data);
	EXPECT_EQ(1, SizeSamplingBuff());
}

TEST_F(SamplingBuffer, TestSetFunctionIncrementsUpToBufferSize_2048)
{
	uint16_t data = 0;
	for(uint32_t i = 0; i < 2048; i++)
	{
		EXPECT_EQ(i, SizeSamplingBuff());
		UplinkInputBufferSet(data);
	}
	UplinkInputBufferSet(data);
	EXPECT_EQ(2048, SizeSamplingBuff());
}


TEST_F(SamplingBuffer, TestNullPointerPassedIntoGetFunctionReturnsZero)
{
	uint16_t data = 0;
	UplinkInputBufferSet(data);
	EXPECT_EQ(0, UplinkInputBufferGet(0));
}

TEST_F(SamplingBuffer, TestSingleCharCanBePlacedAndThenRemovedFromBuffer)
{
	uint16_t dataIn = 123;
	float dataOut = 0;
	UplinkInputBufferSet(dataIn);	
	UplinkInputBufferGet(&dataOut);
	EXPECT_FLOAT_EQ(123, dataOut);
}

TEST_F(SamplingBuffer, Test2000CharsCanBePlacedAndThenRemovedFromBuffer)
{
	float dataOut = 0;
	for(uint32_t i = 0; i < 2048; i++)
	{
		UplinkInputBufferSet(i);	
	}
	for(uint32_t i = 0; i < 2048; i++)
	{
		UplinkInputBufferGet(&dataOut);
		EXPECT_FLOAT_EQ(i, dataOut);
	}
}

TEST_F(SamplingBuffer, Test2000CharsCanBePlacedAndThenRemovedFromBufferMultipleTimes)
{
	float dataOut = 0;
	for(uint32_t i = 0; i < 2048; i++)
	{
		UplinkInputBufferSet(i);	
	}
	for(uint32_t i = 0; i < 2048; i++)
	{
		UplinkInputBufferGet(&dataOut);
		EXPECT_FLOAT_EQ(i, dataOut);
	}

	for(uint32_t i = 0; i < 2048; i++)
	{
		UplinkInputBufferSet(i);	
	}
	for(uint32_t i = 0; i < 2048; i++)
	{
		UplinkInputBufferGet(&dataOut);
		EXPECT_FLOAT_EQ(i, dataOut);
	}

}

TEST_F(SamplingBuffer, TestCountDecrementsWhenWeTakeFromBuffer)
{
	uint16_t dataIn = 123;
	float dataOut = 0;
	UplinkInputBufferSet(dataIn);	
	UplinkInputBufferGet(&dataOut);
	EXPECT_EQ(0, SizeSamplingBuff());
}