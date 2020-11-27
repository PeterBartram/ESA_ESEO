
#include "usartDriver.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pdca.h"

static void USARTinterruptHandler(void);

#define USART_INTERRUPT_PRIORITY 3
#define USART_PDCA_CHANNEL 1


void USARTinitialise(void)
{	
	static const gpio_map_t USART_GPIO_MAP =
	{
		{EXAMPLE_USART_RX_PIN, EXAMPLE_USART_RX_FUNCTION},
		{EXAMPLE_USART_TX_PIN, EXAMPLE_USART_TX_FUNCTION}
	};

	// USART options.
	static const usart_options_t USART_OPTIONS =
	{
		.baudrate     = 512000,
		.charlength   = 8,
		.paritytype   = USART_NO_PARITY,
		.stopbits     = USART_1_STOPBIT,
		.channelmode  = USART_NORMAL_CHMODE
	};	
		
	// Assign GPIO to USART.
	gpio_enable_module(USART_GPIO_MAP,
	sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));

	// Initialize USART in RS232 mode.
	usart_init_rs232(EXAMPLE_USART, &USART_OPTIONS, CLK_PBA_F);
	
	INTC_register_interrupt(&USARTinterruptHandler, AVR32_PDCA_IRQ_1, USART_INTERRUPT_PRIORITY);
	INTC_register_interrupt(&USARTinterruptHandler, AVR32_USART2_IRQ, USART_INTERRUPT_PRIORITY);
}



volatile avr32_usart_t *USARTinterface;
uint8_t * USARTdata;
uint16_t  USARTdataSize;
uint8_t	  USARTtransferComplete;
uint16_t  USARTbytesSent;
 
//uint32_t count = 0;
uint32_t transfers = 0;

 __attribute__((__interrupt__))
static void USARTinterruptHandler(void)
{
	pdca_disable(USART_PDCA_CHANNEL);
	pdca_disable_interrupt_transfer_complete(USART_PDCA_CHANNEL);
	USARTtransferComplete = 1;
	//count++;
}

void USARTblockWrite(volatile avr32_usart_t *usart, uint8_t * z_data, uint16_t z_size)
{
	USARTtransferComplete = 0;
	
	/* Configure the DMA channel for a single transfer. */
	pdca_channel_options_t DMAConfigurationOptions;
	DMAConfigurationOptions.addr = (void*)z_data;							/* Copy across trace data */
	DMAConfigurationOptions.pid = AVR32_PDCA_PID_USART2_TX;					/* Write to USART#2 transmit register. */
	DMAConfigurationOptions.size = z_size;									/* Write all data */
	DMAConfigurationOptions.transfer_size = PDCA_TRANSFER_SIZE_BYTE;		/* Copy across data in 16 bit half words. */
	DMAConfigurationOptions.r_addr = 0;										/* For chaining transfers - not required. */
	DMAConfigurationOptions.r_size = 0;										/* For chaining transfers - not required. */
	
	/* Initialise the DMA transfer. */
	pdca_init_channel(USART_PDCA_CHANNEL, &DMAConfigurationOptions);
	
	pdca_enable_interrupt_transfer_complete(USART_PDCA_CHANNEL);
	
	/* Enable this DMA channel. */
	pdca_enable(USART_PDCA_CHANNEL);
	transfers++;
	
	while(!USARTtransferComplete)
	{
		vTaskDelay(10);
	}
}

/********************/
/* Interrupt driven */
/********************/

//void USARTinitialise(void)
//{	
	//static const gpio_map_t USART_GPIO_MAP =
	//{
		//{EXAMPLE_USART_RX_PIN, EXAMPLE_USART_RX_FUNCTION},
		//{EXAMPLE_USART_TX_PIN, EXAMPLE_USART_TX_FUNCTION}
	//};
//
	//// USART options.
	//static const usart_options_t USART_OPTIONS =
	//{
		//.baudrate     = 512000,
		//.charlength   = 8,
		//.paritytype   = USART_NO_PARITY,
		//.stopbits     = USART_1_STOPBIT,
		//.channelmode  = USART_NORMAL_CHMODE
	//};	
		//
	//// Assign GPIO to USART.
	//gpio_enable_module(USART_GPIO_MAP,
	//sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));
//
	//// Initialize USART in RS232 mode.
	//usart_init_rs232(EXAMPLE_USART, &USART_OPTIONS, CLK_PBA_F);
	//
	//INTC_register_interrupt(&USARTinterruptHandler, AVR32_USART2_IRQ, USART_INTERRUPT_PRIORITY);
//}
//
//
//
//volatile avr32_usart_t *USARTinterface;
//uint8_t * USARTdata;
//uint16_t  USARTdataSize;
//uint8_t	  USARTtransferComplete;
//uint16_t  USARTbytesSent;
 //
//uint32_t count = 0;
//
 //__attribute__((__interrupt__))
//static void USARTinterruptHandler(void)
//{
	//count ++;
	//if(USARTinterface && USARTdata)
	//{
		//if(USARTbytesSent < USARTdataSize)
		//{
			//usart_write_char(USARTinterface, (int)USARTdata[USARTbytesSent]);
			//USARTbytesSent++;
			//
			//if(USARTbytesSent >= USARTdataSize)
			//{
				//USARTtransferComplete = 1;
				//EXAMPLE_USART->idr = AVR32_USART_IER_TXEMPTY_MASK;		
			//}
		//}
	//}
//}
//
//void USARTblockWrite(volatile avr32_usart_t *usart, uint8_t * z_data, uint16_t z_size)
//{
	//if(usart && z_data && z_size > 0)
	//{
		//USARTtransferComplete = 0;
		//USARTdataSize = z_size;
		//USARTdata = z_data;
		//USARTinterface = usart;
		//USARTbytesSent = 0;
		//
		//EXAMPLE_USART->ier = AVR32_USART_IER_TXEMPTY_MASK;
		//
		//while(!USARTtransferComplete)
		//{
			//vTaskDelay(1);
		//}
	//}
//}
