#include <stdint.h>
#include <stdio.h>
#include <tchar.h>

#include "gtest/gtest.h"
extern "C"
{
	//#include "Uplink.c"
}

//void(*p_uplinkExecute)(void) = UplinkExecute;

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}