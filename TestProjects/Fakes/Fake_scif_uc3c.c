#include "Fake_scif_uc3c.h"

DEFINE_FAKE_VALUE_FUNC4(long int, scif_gc_setup, unsigned int, scif_gcctrl_oscsel_t, unsigned int, unsigned int);
DEFINE_FAKE_VALUE_FUNC1(long int, scif_gc_enable, unsigned int);