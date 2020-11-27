/**
@file 	CANDriver.c
@author Pete Bartram
@brief 
This driver will handle sending and receiving of CAN packets through its API.
This driver is also responsible for handling switching between the two onboard CAN channels.

@todo Error handling and channel switching.
*/
#include "CANDriver.h"
#include "asf.h"
#include "scif_uc3c.h"		// System Control Interface (SCIF) - Used for handling PLLs and Generic Clocks for Modules
#include "string.h"
#include "RingBuffer.h"
#include "delay.h"

/* Permanent defines - dont't change these! */
#define MAX_MESSAGE_LENGTH 8				
#define NUMBER_MOBS_PER_CHANNEL 16					/**< Number of message objects available per channel. (Fixed at 16 for this processor). */
#define	TRANSMIT_MOBS_PER_CHANNEL 4					/**< Number of MOBs dedicated to transmission. */
#define	RECEIVE_MOBS_PER_CHANNEL 12					/**< Number of MOBs dedicated to data reception */
#define NUMBER_OF_CHANNELS 2						/**< Number of CAN channels. */
#define TRANSMIT_MOB_TABLE_FULL_MASK	0x0000000F	/**< What the transmit portion of the allocation table looks like when full */
#define COB_ID_FILTER_MASK	0x3FF					/**< Mask such that we ignore all but the first 11 bits of the COB ID */

/* Configuration defines. */
#define RECEIVE_PACKET_BUFFER_SIZE 10		/**< Number of packets that can go into the ring buffer. */

/* Interrupt event priority levels */
#define CAN_INTERRUPT_LEVEL_TX	1
#define CAN_INTERRUPT_LEVEL_RX	1
#define CAN_INTERRUPT_LEVEL_BOFF	1
#define CAN_INTERRUPT_LEVEL_ERR	1
#define CAN_INTERRUPT_LEVEL_WAKEUP	1

/* Variable declarations. */
volatile static can_msg_t messageObjectBuffer[NUMBER_MOBS_PER_CHANNEL];				/**< Message object buffer. */
static uint32_t messageObjectAllocationTable = 0;									/**< Bitwise MOB allocation table. */
static CAN_PACKET receivePacketBuffer[RECEIVE_PACKET_BUFFER_SIZE];					/**< Memory for receive ring buffer. */
static uint32_t packetsDropped;														/**< Count of packets dropped. */
static uint64_t packetsReceived;													/**< Count of packets received. */
static uint32_t failedTransmission;													/**< Count of the number of packets that could not be sent due to lack of buffer space. */
static uint64_t packetsTransmitted;													/**< Count of packets successfully transmitted. */
static uint8_t canChannelNumber;													/**< The current CAN channel number */

/* Private functions. */
static void ConfigureReceiveMessageObject(uint8_t z_channel, uint32_t z_handle, uint16_t z_cobID);
static CAN_ERROR AllocateTransmitMessageObject(uint8_t z_channel, uint32_t * z_handle);
static CAN_ERROR AllocateReceiveMessageObject(uint8_t z_channel, uint32_t * z_handle);
static CAN_ERROR FreeMessageObject(uint8_t z_channel, uint32_t z_handle);
static void InitialiseReceive(uint16_t * z_receiveCODIDs, uint8_t z_numberOfReceiveCOBIDs);
static void ClearMOBAllocations(uint8_t z_channel);
static CAN_ERROR ConfigureSingleCANChannel(uint8_t z_channel);

/* Interrupt service routines. */
static void CANTransmitOK_ISR(void);
static void CANReceiveOK_ISR(void);
static void CANBusOff_ISR(void);
static void CANError_ISR(void);
static void CANWakeUp_ISR(void);

/* Data map of the output CAN pins. */
const gpio_map_t CAN_GPIO_MAP = {
	{AVR32_CANIF_RXLINE_0_0_PIN, AVR32_CANIF_RXLINE_0_0_FUNCTION},
	{AVR32_CANIF_TXLINE_0_0_PIN, AVR32_CANIF_TXLINE_0_0_FUNCTION},
	{AVR32_CANIF_RXLINE_1_2_PIN, AVR32_CANIF_RXLINE_1_2_FUNCTION},
	{AVR32_CANIF_TXLINE_1_2_PIN, AVR32_CANIF_TXLINE_1_2_FUNCTION}
};	

uint16_t * receiveCODIDs = 0;
uint8_t numberOfReceiveCOBIDs = 0;

/**
*	Perform initalisation of both CAN channels, allocate buffers and configure interrupts.
*	\param[in] z_receiveCODIDs: COB-IDs to configure. These are stored locally and MUST be statically allocated!!!
*	\param[in] z_numberOfReceiveCOBIDs: Number of COB-IDs
*	\return NO_ERROR_CAN or INITIALISATION_FAILED
*/
CAN_ERROR InitialiseCANDriver(uint16_t * z_receiveCODIDs, uint8_t z_numberOfReceiveCOBIDs)
{
	CAN_ERROR retVal = NO_ERROR_CAN;

	if(z_receiveCODIDs)
	{
		/* Ensure that we only continue for a correct number of COB-IDs */
		if(z_numberOfReceiveCOBIDs <= RECEIVE_MOBS_PER_CHANNEL)
		{
			/* Assign the GPIO output pins to operate as CAN TX and RX pins - can only fail from bad inputs. */
			if(gpio_enable_module(CAN_GPIO_MAP, sizeof(CAN_GPIO_MAP) / sizeof(CAN_GPIO_MAP[0])) == GPIO_SUCCESS)
			{
				/* The CAN modules needs a 'generic clock' input signal as the module is not provided with
				   a clock by default, this function will ensure that it gets the necessary clock speed configured. */
				scif_gc_setup(AVR32_SCIF_GCLK_CANIF,
				SCIF_GCCTRL_OSC0,
				AVR32_SCIF_GC_NO_DIV_CLOCK,
				0);
	
				/* Now we need to enable the clock - CAN module is now live. */
				scif_gc_enable(AVR32_SCIF_GCLK_CANIF);	
				
				/* Configure to begin on channel zero */
				canChannelNumber = 0;						/* The current channel needs to be stored in non.vol otherwise after a reset we could cause problems. */
				
				/* Configure a single CAN channel */
				ConfigureSingleCANChannel(canChannelNumber);				

				/* As we are only using a single channel at once all interrupts direct to the same ISRs */
				/* Channel zero interrupt configuration. */
				INTC_register_interrupt(&CANTransmitOK_ISR, AVR32_CANIF_TXOK_IRQ_0, CAN_INTERRUPT_LEVEL_TX);
				INTC_register_interrupt(&CANReceiveOK_ISR, AVR32_CANIF_RXOK_IRQ_0, CAN_INTERRUPT_LEVEL_RX);
				INTC_register_interrupt(&CANBusOff_ISR, AVR32_CANIF_BUS_OFF_IRQ_0, CAN_INTERRUPT_LEVEL_BOFF);
				INTC_register_interrupt(&CANError_ISR, AVR32_CANIF_ERROR_IRQ_0, CAN_INTERRUPT_LEVEL_ERR);
				INTC_register_interrupt(&CANWakeUp_ISR, AVR32_CANIF_WAKE_UP_IRQ_0, CAN_INTERRUPT_LEVEL_WAKEUP);

				/* Channel one interrupt configuration. */
				INTC_register_interrupt(&CANTransmitOK_ISR, AVR32_CANIF_TXOK_IRQ_1, CAN_INTERRUPT_LEVEL_TX);
				INTC_register_interrupt(&CANReceiveOK_ISR, AVR32_CANIF_RXOK_IRQ_1, CAN_INTERRUPT_LEVEL_RX);
				INTC_register_interrupt(&CANBusOff_ISR, AVR32_CANIF_BUS_OFF_IRQ_1, CAN_INTERRUPT_LEVEL_BOFF);
				INTC_register_interrupt(&CANError_ISR, AVR32_CANIF_ERROR_IRQ_1, CAN_INTERRUPT_LEVEL_ERR);
				INTC_register_interrupt(&CANWakeUp_ISR, AVR32_CANIF_WAKE_UP_IRQ_1, CAN_INTERRUPT_LEVEL_WAKEUP);
				
				/* Enable CAN interrupts. */
				CANIF_enable_interrupt(canChannelNumber);
	
				/* Take a copy of our COB-IDs for future reference */
				receiveCODIDs = z_receiveCODIDs;
				numberOfReceiveCOBIDs = z_numberOfReceiveCOBIDs;

				/* Configure based upon our COB-IDs */
				InitialiseReceive(z_receiveCODIDs, z_numberOfReceiveCOBIDs);
			}
		}
		else
		{
			retVal = TOO_MANY_RECEVIE_COB_IDS;
		}
	}
	else
	{
		retVal = NULL_POINTER;
	}

	return retVal;
}

/**
*	Configure a single CAN channels registers. 
*	\param[in] z_channel : the channel to configure.
*	\return NO_ERROR_CAN or INITIALISATION_FAILED
*/
static CAN_ERROR ConfigureSingleCANChannel(uint8_t z_channel)
{
	CAN_ERROR retVal = NO_ERROR_CAN;
	
	/* We begin by disabling the CAN channel (just in case). */
	CANIF_disable(z_channel);
				
	/* Reset the channel - clears all user interface registers */
	CANIF_set_reset(z_channel);
				
	/* We must wait until the channel is marked as disabled before continuing. */
	while(CANIF_channel_enable_status(z_channel)); ///@todo add a time out here.
				
	/* Clear the CAN initalisation bit (cannot continue until this is done) */
	CANIF_clr_reset(z_channel);

	/* Set up our pointers to the message object buffers. */
	CANIF_set_ram_add(z_channel, (unsigned long)messageObjectBuffer);
				
	/* Configure the baud rate for communications - 250kbps */
	CANIF_set_sjw(z_channel, 1);
	CANIF_set_prs(z_channel, 5);
	CANIF_set_pres(z_channel, 3);
	CANIF_set_phs2(z_channel, 2);
	CANIF_set_phs1(z_channel, 5);
				
	/* Configure the channel to operate in NORMAL mode. */
	CANIF_set_channel_mode(z_channel, CANIF_CHANNEL_MODE_NORMAL);

	/* We do not want to send overrun packets - so we disable this here. */
	CANIF_clr_overrun_mode(z_channel);
				
	/* Clear all of this channels message objects. */
	canif_clear_all_mob(z_channel, NUMBER_MOBS_PER_CHANNEL);
				
	/* Enable this channel. */
	CANIF_enable(z_channel);

	/* Need to allow the bus to perform arbitration before we check that the channel was successfully enabled. */
	#define DELAY_HZ         (BAUDRATE_HZ/141.0)   /*Compute Maximum delay time*/
	#define DELAY            (1000000 / DELAY_HZ)  /*Compute Delay in µs*/
	delay_us(DELAY);
				
	if(!CANIF_channel_enable_status(z_channel))
	{
		/* Need to do something with this. */
		retVal = INITIALISATION_FAILED;
	}	
	return retVal;
}

/**
*	Perform initalisation of the message objects for the receive channel.
*	We just listen on all mobs for all packets and filter later.
*	\param[in] z_receiveCODIDs: COB-IDs to configure
*	\param[in] z_numberOfReceiveCOBIDs: Number of COB-IDs
*/
static void InitialiseReceive(uint16_t * z_receiveCODIDs, uint8_t z_numberOfReceiveCOBIDs)
{
	uint32_t handle = 0;
	uint32_t i = 0;
			
	/* This has already been null checked in the calling function */
	if(z_receiveCODIDs)
	{
		/* Initialise the CAN packet reception ring buffer. */
		RingBufferInit(receivePacketBuffer, sizeof(receivePacketBuffer)/sizeof(receivePacketBuffer[0]));

		for(i = 0; i < z_numberOfReceiveCOBIDs; i++)
		{
			/* Allocate a MOB for transmission */
			if(AllocateReceiveMessageObject(canChannelNumber, &handle) == NO_ERROR_CAN)
			{
				ConfigureReceiveMessageObject(canChannelNumber, handle, z_receiveCODIDs[i]);
			}
		}

		/* Reset our packet counters */
		packetsDropped = 0;
		packetsReceived = 0;
	}
}

/**
*	Handle register level configuration of a single receive MOB.
*	\param[in] z_channel: Channel to configure
*	\param[in] z_handle: MOB handle to configure
*/
static void ConfigureReceiveMessageObject(uint8_t z_channel, uint32_t z_handle, uint16_t z_cobID)
{
	CANIF_set_std_id(z_channel, z_handle, z_cobID);
	CANIF_set_std_idmask(z_channel, z_handle, COB_ID_FILTER_MASK);	/* We only care about the first 11 identifier bits in the COB-ID */
	CANIF_config_rx(z_channel,z_handle);
	CANIF_mob_enable(z_channel,z_handle);
	CANIF_mob_enable_interrupt(z_channel,z_handle);	
}


/**
*	Interface function - transmit a given CAN packet.
*	There is room for 8 packets to be transmitted at once, anymore that this and packets will be dropped.
*	\param[in] z_channel: Packet to transmit.
*	\return NO_ERROR_CAN if packet was accepted for transmission or ERROR_MOB_NOT_AVALIABLE if there was not space for the transmission.
*/
CAN_ERROR TransmitCAN(CAN_PACKET z_packet)
{
	CAN_ERROR retVal = NO_ERROR_CAN;
	uint32_t handle = 0;
	uint64_t buffer = 0;
	
	if(z_packet.dataSize > 0 && z_packet.dataSize <= MAX_MESSAGE_LENGTH)
	{
		/* Allocate a MOB for transmission */
		if(AllocateTransmitMessageObject(canChannelNumber, &handle) == NO_ERROR_CAN)
		{
			CANIF_set_std_id(canChannelNumber, handle, z_packet.id);
			CANIF_mob_clr_dlc(canChannelNumber, handle);
			CANIF_mob_set_dlc(canChannelNumber, handle, z_packet.dataSize);
			
			/* Copy data into correct format for the interface function. */
			memcpy(&buffer, z_packet.data, MAX_MESSAGE_LENGTH);
			CANIF_set_data(canChannelNumber, handle, buffer);
			CANIF_config_tx(canChannelNumber, handle);
			CANIF_mob_enable(canChannelNumber, handle);
			CANIF_mob_enable_interrupt(canChannelNumber, handle);
		}
		else
		{
			retVal = ERROR_MOB_NOT_AVALIABLE;
		}
	}
	else
	{
		retVal = ERROR_DATA_LENGTH_TOO_LONG;
	}
	return retVal;
}

/**
*	Interface function - check buffer for any received packets.
*	There is room for 8 packets to be transmitted at once, anymore that this and packets will be dropped.
*	\param[out] z_packet Packet that has been received.
*	\return NO_ERROR_CAN if packet has been received or NO_PACKETS_RECEIVED if there are no new packets.
*/
CAN_ERROR ReceiveCAN(CAN_PACKET * z_packet)
{
	CAN_ERROR retVal = NO_ERROR_CAN;
	CAN_PACKET buffer;

	if(z_packet)
	{
		if(!RingBufferIsEmpty())
		{
			buffer = RingBufferGetElement();
			memcpy(z_packet, &buffer, sizeof(CAN_PACKET));
		}
		else
		{
			retVal = NO_PACKETS_RECEIVED;
		}
	}
	else
	{
		retVal = NULL_POINTER;
	}
	return retVal;
}

/**
*	Change to the other available CAN channel.
*	\param[in] z_receiveCODIDs: COB-IDs to configure
*	\param[in] z_numberOfReceiveCOBIDs: Number of COB-IDs
*	\return NO_ERROR_CAN or INITIALISATION_FAILED
*/
CAN_ERROR ChangeCANChannel(void)
{
	CAN_ERROR retVal = NO_ERROR_CAN;
	
	/* Disable interrupts on our current channel */
	CANIF_disable_interrupt(canChannelNumber);
	
	/* Disable our current channel. */
	CANIF_disable(canChannelNumber);
	
	/* Clear all of the MOBs in use for the current channel */
	ClearMOBAllocations(canChannelNumber);
	
	/* Change the channel. */
	if(canChannelNumber == 0) 
	{
		canChannelNumber = 1;
	}
	else 
	{
		canChannelNumber = 0;
	}
	
	/* Configure a single CAN channel */
	retVal = ConfigureSingleCANChannel(canChannelNumber);
				
	InitialiseReceive(receiveCODIDs, numberOfReceiveCOBIDs);	
	
	/* Enable CAN interrupts. */
	CANIF_enable_interrupt(canChannelNumber);	
	
	return retVal;
}

/**
*	Interface function - check if there is a CAN transmit slot available.
*	\return Non-zero if there is a transmit slot available to be used, else zero.
*/
uint8_t CANHasTransmitSpaceAvailable(void)
{
	if((messageObjectAllocationTable & TRANSMIT_MOB_TABLE_FULL_MASK) != TRANSMIT_MOB_TABLE_FULL_MASK)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/* Diagnostics functions. */
uint32_t PacketsDropped(void)
{
	return packetsDropped;
}

void ResetPacketsDroppedCount(void)
{
	packetsDropped = 0;
}

uint64_t PacketsReceived(void)
{
	return packetsReceived;
}

void ResetPacketsReceivedCount(void)
{
	packetsReceived = 0;
}

uint32_t PacketsTransmitted(void)
{
	return packetsTransmitted;
}

void ResetPacketsTransmittedCount(void)
{
	packetsTransmitted = 0;
}

uint64_t FailedTransmissions(void)
{
	return failedTransmission;
}

void ResetFailedTransmission(void)
{
	failedTransmission = 0;
}

/**
*	Allocate a MOB to use for transmission
*	\param[in] z_channel: Channel to allocate a MOB from
*	\param[out] z_handle: Returns the MOB handle that was allocated.
*	\return CAN_ERROR
*/
static CAN_ERROR AllocateTransmitMessageObject(uint8_t z_channel, uint32_t * z_handle)
{
	CAN_ERROR retVal = NO_ERROR_CAN;
	uint32_t i = 0;
	
	if(z_handle)
	{
		/* Ensure we aren't accessing a channel that doesn't exist. */
		if(z_channel <= NUMBER_OF_CHANNELS)
		{
			/* Assume failure until we find a free MOB. */
			retVal = ERROR_MOB_NOT_AVALIABLE;

			/* Loop for each potential MOB */
			for(i = 0; i < TRANSMIT_MOBS_PER_CHANNEL; i++)
			{
				/* Check if this mob has been previously allocated. */
				if(!((messageObjectAllocationTable >> i) & 0x1))
				{
					/* Mark the MOB as taken and clear out the data in it. */
					messageObjectAllocationTable |= (0x01 << i);
					CANIF_clr_mob(z_channel, i);
					*z_handle = i;
					retVal = NO_ERROR_CAN;
					break;
				}
			}
		}
		else 
		{
			retVal = ERROR_CHANNEL_NUMBER;
		}
	}
	else
	{
		retVal = NULL_POINTER;
	}
	return retVal;
}

/**
*	Allocate a MOB to use for reception
*	\param[in] z_channel: Channel to allocate a MOB from
*	\param[out] z_handle: Returns the MOB handle that was allocated.
*	\return CAN_ERROR
*/
static CAN_ERROR AllocateReceiveMessageObject(uint8_t z_channel, uint32_t * z_handle)
{
	CAN_ERROR retVal = NO_ERROR_CAN;
	uint32_t i = 0;
	
	if(z_handle)
	{
		/* Ensure we aren't accessing a channel that doesn't exist. */
		if(z_channel <= NUMBER_OF_CHANNELS)
		{
			/* Assume failure unless we find a MOB. */
			retVal = ERROR_MOB_NOT_AVALIABLE;

			/* Loop for each potential MOB */
			for(i = 0; i < RECEIVE_MOBS_PER_CHANNEL; i++)
			{
				/* Check if this mob has been previously allocated. */
				if(!((messageObjectAllocationTable >> (i + TRANSMIT_MOBS_PER_CHANNEL)) & 0x1))
				{
					/* Mark the MOB as taken and clear out the data in it. */
					messageObjectAllocationTable |= (0x01 << (i + TRANSMIT_MOBS_PER_CHANNEL));
					CANIF_clr_mob(z_channel, (i + TRANSMIT_MOBS_PER_CHANNEL));
					*z_handle = (i + TRANSMIT_MOBS_PER_CHANNEL);
					retVal = NO_ERROR_CAN;
					break;
				}
			}
		}
		else
		{
			retVal = ERROR_CHANNEL_NUMBER;
		}
	}
	else
	{
		retVal = NULL_POINTER;
	}
	return retVal;
}

/**
*	Free a MOB such that it can be used again.
*	\param[in] z_channel: Channel to free the MOB from.
*	\param[in] z_handle: Handle to MOB to free.
*	\return CAN_ERROR
*/
static CAN_ERROR FreeMessageObject(uint8_t z_channel, uint32_t z_handle)
{
	CAN_ERROR retVal = NO_ERROR_CAN;
	uint32_t i = 0;
	
	/* Ensure we aren't accessing a channel that doesn't exist. */
	if(z_channel < NUMBER_OF_CHANNELS)
	{
		/* Ensure we have a valid handle. */
		if(z_handle < NUMBER_MOBS_PER_CHANNEL)
		{
			/* Free the message object */
			messageObjectAllocationTable &= (~(0x01 << z_handle));
		}
		else
		{
			retVal = ERROR_INVALID_MOB_HANDLE;
		}
	}
	else
	{
		retVal = ERROR_CHANNEL_NUMBER;
	}
	
	return retVal;
}

/**
*	Disable MOBs for the specified channel and clear the message allocation table
*	\param[in] z_channel: Channel to disable all MOBs from.
*/
static void ClearMOBAllocations(uint8_t z_channel)
{
	uint8_t i = 0;
	
	/* Clear our MOB allocation table. */
	messageObjectAllocationTable = 0;
	
	/* Disable each MOB individually for the active channel. */
	for(i = 0; i <NUMBER_MOBS_PER_CHANNEL; i++)
	{
		CANIF_mob_disable_interrupt(z_channel, i);
		CANIF_mob_disable(z_channel, i);
	}
}

/**
*	Interrupt Service Routine
*	Called when a CAN packet is successfully transmitted - handles clearing the associated MOB.
*/
__attribute__((interrupt("full")))
static void CANTransmitOK_ISR(void)
{
	uint8_t handle;
	
	/* Get the handle of the MOB that has been transmitted. */
	handle = CANIF_mob_get_mob_txok(canChannelNumber);
	
	/* Clear the TX status of the transmitted MOB. */
	CANIF_mob_clear_txok_status(canChannelNumber, handle);
	
	/* Clear the MOB status for reuse. */
	CANIF_mob_clear_status(canChannelNumber, handle);
	
	/* Free the MOB so that it can be used again. */
	FreeMessageObject(canChannelNumber, handle);
}

/**
*	Interrupt Service Routine
*	Called when a CAN packet is received. Places received packets into a ring buffer for later consumption,
*	frees the MOB that received the packet and then reallocates the MOB to listen on the CAN bus again.
*/
__attribute__((interrupt("full")))
static void CANReceiveOK_ISR(void)
{
	uint32_t handle;
	uint32_t dataSize = 0;
	CAN_PACKET buffer;
	uint16_t cobID = 0;
	
	/* Get the handle of the MOB that has received a packet. */
	handle = CANIF_mob_get_mob_rxok(canChannelNumber);
	
	/* Get the COB-ID of the MOB we have received a message on for later reuse. */
	cobID = CANIF_get_ext_id(canChannelNumber, handle);
	
	/* Store the size of the data portion of the packet received. */
	buffer.dataSize = CANIF_mob_get_dlc(canChannelNumber,handle);
	
	if(buffer.dataSize <= MAX_MESSAGE_LENGTH)  
	{
		/* Copy the data out of the packet. */
		buffer.id = messageObjectBuffer[handle].id;
		memcpy(buffer.data, (void*)&messageObjectBuffer[handle].data, buffer.dataSize);
	
		/* Put the contents of the received packet into the ring buffer */
		if(!RingBufferIsFull())
		{
			if(RingBufferAddElement(&buffer))
			{
				packetsReceived++;
			}
			else
			{
				packetsDropped++;
			}
		}
	}
	
	/* Clear the TX status of the transmitted MOB. */
	CANIF_mob_clear_rxok_status(canChannelNumber, handle);
	
	/* Clear the MOB status for reuse. */
	CANIF_mob_clear_status(canChannelNumber, handle);
	
	/* Free the MOB so that it can be used again. */
	FreeMessageObject(canChannelNumber, handle);
	
	/* Allocate the next (probably the same) MOB for reception */
	if(AllocateReceiveMessageObject(canChannelNumber, &handle) == NO_ERROR_CAN)
	{
		/* Configure this message with the same COB-ID as we received on. */
		ConfigureReceiveMessageObject(canChannelNumber, handle, cobID);
	}
	else
	{
		/* @todo ROLL THIS INTO THE ERROR HANDLING SECTION ISR WORK */
	}
}

__attribute__((interrupt("full")))
static void CANBusOff_ISR(void)
{
	/* @todo NEED A GOOD ERROR HANDLING STORY HERE. */
	CANIF_clr_interrupt_status(canChannelNumber);
}

__attribute__((interrupt("full")))
static void CANError_ISR(void)
{
	/* @todo NEED A GOOD ERROR HANDLING STORY HERE. */
	CANIF_clr_interrupt_status(canChannelNumber);
}

__attribute__((interrupt("full")))
static void CANWakeUp_ISR(void)
{
	/* We dont need to do anything here - we weren't napping! */
}
