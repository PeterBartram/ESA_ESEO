#ifndef FAKE_SCIF
#define FAKE_SCIF

#include "scif_uc3c.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC4(long int, scif_gc_setup, unsigned int, scif_gcctrl_oscsel_t, unsigned int, unsigned int);
DECLARE_FAKE_VALUE_FUNC1(long int, scif_gc_enable, unsigned int);


#endif