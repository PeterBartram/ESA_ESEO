#include <stdint.h>
#include <stdio.h>
#include <tchar.h>

#include "gtest/gtest.h"

extern "C"
{
	#include "SatelliteOperations.c"
	#include "WOD.c"
	#include "DefaultValues.c"
	#include "OperatingModes.c"
}
void (*p_WodTakeSample)(xTimerHandle z_timer) = WodTakeSample;
void (*p_updateDefaultValues)(xTimerHandle z_timer) = UpdateDefaultValuesCallBack;
void (*p_sampleSequenceNumber)(xTimerHandle z_timer) = SampleSequenceNumber;
uint8_t * p_eclipseTimeExpiredFlag = &eclipseTimeExpiredFlag;

DEFAULT_VALUES_STRUCTURE * p_payloadDataStore = &payloadDataStore[0];

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}