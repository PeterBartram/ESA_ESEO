#ifndef INTC_H
#define INTC_H
#include <stdint.h>

void INTC_register_interrupt(void(*z_handler)(void), uint32_t z_irq, uint32_t z_intLevel);

#endif