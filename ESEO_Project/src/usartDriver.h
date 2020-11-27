/*
 * usartDriver.h
 *
 * Created: 16-Jun-16 3:38:36 PM
 *  Author: Pete
 */ 


#ifndef USARTDRIVER_H_
#define USARTDRIVER_H_

/* Temporary driver for the USART for testing purposes only - this is not flight code. */
#include <avr32/io.h>
#include "compiler.h"
#include "board.h"
#include "conf_clock.h"
#include "gpio.h"
#include "usart.h"

#define EXAMPLE_USART                 (&AVR32_USART2)
#define EXAMPLE_USART_RX_PIN          AVR32_USART2_RXD_0_1_PIN
#define EXAMPLE_USART_RX_FUNCTION     AVR32_USART2_RXD_0_1_FUNCTION
#define EXAMPLE_USART_TX_PIN          AVR32_USART2_TXD_0_1_PIN
#define EXAMPLE_USART_TX_FUNCTION     AVR32_USART2_TXD_0_1_FUNCTION
#define EXAMPLE_USART_CLOCK_MASK      AVR32_USART2_CLK_PBA
#define EXAMPLE_PDCA_CLOCK_HSB        AVR32_PDCA_CLK_HSB

void USARTinitialise(void);
void USARTblockWrite(volatile avr32_usart_t *usart, uint8_t * z_data, uint16_t z_size);



#endif /* USARTDRIVER_H_ */