#include "Fake_TwoWireInterface.h"

DEFINE_FAKE_VALUE_FUNC2(status_code_t, twim_master_init, volatile avr32_twim_t *, twim_options_t *);
DEFINE_FAKE_VALUE_FUNC2(status_code_t, twim_write_packet,volatile avr32_twim_t *, const twim_package_t *);
DEFINE_FAKE_VALUE_FUNC2(status_code_t, twim_read_packet, volatile avr32_twim_t *, const twim_package_t *);