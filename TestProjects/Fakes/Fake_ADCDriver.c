#include "Fake_ADCDriver.h"

DEFINE_FAKE_VALUE_FUNC2(ADC_ERROR, ADCInitialise, uint16_t *, uint32_t);
DEFINE_FAKE_VALUE_FUNC0(ADC_ERROR, FetchDiagnosticsData);
DEFINE_FAKE_VOID_FUNC0(ADCUninitialise);