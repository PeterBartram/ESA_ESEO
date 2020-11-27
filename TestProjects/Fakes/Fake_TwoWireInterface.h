#ifndef _FAKE_TWO_WIRE_INTERFACE_H
#define _FAKE_TWO_WIRE_INTERFACE_H

#include "twim.h"
#include "asf.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC2(status_code_t, twim_master_init, volatile avr32_twim_t *, twim_options_t *);
DECLARE_FAKE_VALUE_FUNC2(status_code_t, twim_write_packet, volatile avr32_twim_t *, const twim_package_t *);
DECLARE_FAKE_VALUE_FUNC2(status_code_t, twim_read_packet, volatile avr32_twim_t *, const twim_package_t *);

#endif