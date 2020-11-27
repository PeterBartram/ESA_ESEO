#ifndef GPIO_FAKE_H
#define GPIO_FAKE_H

#include "fff.h"
#include "gpio.h"

DECLARE_FAKE_VALUE_FUNC2(uint32_t, gpio_enable_module, gpio_map_t *, uint32_t);
DECLARE_FAKE_VOID_FUNC2(gpio_configure_pin, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC1(gpio_set_pin_low, uint32_t);
DECLARE_FAKE_VOID_FUNC1(gpio_set_pin_high, uint32_t);
DECLARE_FAKE_VALUE_FUNC1(uint8_t, gpio_get_pin_value, uint32_t);


#endif