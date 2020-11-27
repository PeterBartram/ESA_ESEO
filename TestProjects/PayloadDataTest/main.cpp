#include <stdint.h>
#include <stdio.h>
#include <tchar.h>

#include "gtest/gtest.h"

extern "C"
{
	#include "PayloadData.c"
}

PAYLOAD_MEMORY_SECTION * p_payloadDataStore = &payloadDataStore;

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}