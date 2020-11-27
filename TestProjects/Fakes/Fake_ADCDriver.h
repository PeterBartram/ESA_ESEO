#ifndef _FAKE_ADC_DRIVER_H
#define _FAKE_ADC_DRIVER_H

#include "ADCDriver.h"
#include "fff.h"


DECLARE_FAKE_VALUE_FUNC2(ADC_ERROR, ADCInitialise, uint16_t *, uint32_t);
DECLARE_FAKE_VALUE_FUNC0(ADC_ERROR, FetchDiagnosticsData);
DECLARE_FAKE_VOID_FUNC0(ADCUninitialise);
#endif