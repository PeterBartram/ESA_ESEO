#include "gtest/gtest.h"

extern "C"
{
	#include "fff.h"
	#include "BitStream.h"
}

DEFINE_FFF_GLOBALS;


class BitStream : public testing::Test
{
public:
	void SetUp()
	{
	}

	virtual void TearDown() 
	{
	}
};


/***************************************************/
/* General initialisation testing.				   */
/***************************************************/
TEST_F(BitStream, TestThatBitStreamDoesntCrashForNullPointers)
{
	AddToBitStream(0, 0, 0, 0);
}

TEST_F(BitStream, TestThatBitStreamReturnsCorrectlyFor8bitValue)
{
	BITSTREAM_CONTROL bitStream;
	uint8_t out = 0;
	bitStream.bitInput = 0;
	bitStream.byteInput = 0;

	AddToBitStream(&bitStream, &out, 8, 0xAA);
	EXPECT_EQ(0xAA, out);
}

TEST_F(BitStream, TestThatBitStreamReturnsCorrectlyFor16bitValue)
{
	BITSTREAM_CONTROL bitStream;
	uint16_t out = 0;
	bitStream.bitInput = 0;
	bitStream.byteInput = 0;

	AddToBitStream(&bitStream, (uint8_t*)&out, 16, 0x1);
	EXPECT_EQ(0x0100, out);
}

TEST_F(BitStream, TestThatBitStreamReturnsCorrectlyFor13bitValue)
{
	BITSTREAM_CONTROL bitStream;
	uint16_t out = 0;
	bitStream.bitInput = 0;
	bitStream.byteInput = 0;

	AddToBitStream(&bitStream, (uint8_t*)&out, 13, 0x1);
	EXPECT_EQ(32, out);
}

TEST_F(BitStream, TestThatBitStreamReturnsCorrectlyFor32bitValue)
{
	BITSTREAM_CONTROL bitStream;
	uint32_t out = 0;
	bitStream.bitInput = 0;
	bitStream.byteInput = 0;

	AddToBitStream(&bitStream, (uint8_t*)&out, 32, 0x01020304);
	EXPECT_EQ(0x04030201, out);
}

