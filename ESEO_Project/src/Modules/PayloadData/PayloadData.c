
#include "FreeRTOS.h"
#include "task.h"
#include "CANODInterface.h"
#include "PayloadData.h"
#include "flashc.h"
#include "conf_clock.h"
#include <stdint.h>
#include <string.h>
#include "queue.h"
#include "Watchdog.h"

/* Target specific defines. */
#define FLASH_PAGE_SIZE_WORDS			128		// The size in words of a FLASH memory page
#define PACKET_SIZE_IN_WORDS			63		// The amount in words of payload data sent down each transfer.

/* Payload data specific defines. */
#define FLASH_PAYLOAD_STORAGE_SIZE			0x40000															// Amount of memory dedicated to storing payload data. (256k)
#define FLASH_PAYLOD_STORAGE_SIZE_WORDS		(FLASH_PAYLOAD_STORAGE_SIZE / sizeof(uint32_t))					// Amount of memory dedicated to storing payload data in words.
#define NUMBER_OF_PAYLOAD_SLOTS				4																// We level-wear across 4 slots to maximize FLASH life */
#define MAXIMUM_PAYLOAD_SIZE_WORDS			(FLASH_PAYLOD_STORAGE_SIZE_WORDS / NUMBER_OF_PAYLOAD_SLOTS)		// Number of words per payload dedicated to a payload transfer.
#define PAYLOAD_NUMBER_OF_PAGES				(MAXIMUM_PAYLOAD_SIZE_WORDS / FLASH_PAGE_SIZE_WORDS)			// The number of pages per payload slot.
#define FLASH_LOCATION_UNINITIALISED		0xFFFFFFFF
#define MAX_32BIT_VALUE						0xFFFFFFFF
#define BITS_IN_A_BYTE						8

/* Semaphore defines */
#define FLASH_ACCESS_TICKS_TO_WAIT		20		// Number of ticks to wait on the flash access semaphore.

/* Static storage for the queue itself */
static uint8_t queueStorage[PAYLOAD_DATA_QUEUE_LENGTH][PAYLOAD_DATA_QUEUE_ITEM_SIZE];

static QueueHandle_t queueHandle = 0;
static StaticQueue_t queueControl;
static SemaphoreHandle_t flashAccessSemaphore;
static StaticSemaphore_t flashAccessSemaphoreStorage;
static uint8_t transferInProgress = 0;				// Set to true if a transfer is in progress.
static uint8_t slotInUse = 0;						// Contains the slot that we are currently writing to.
static uint32_t transferCount = 0;					// The current number of words that have been transferred.
static uint32_t pageCount = 0;						// The current page number we are writing to.
static uint32_t wordCount = 0;						// The current word number within a page we are writing to.
static uint8_t justWrittenToFlash = 0;				// Set to one if we have just performed a write to flash.	
static uint32_t pageBuffer[FLASH_PAGE_SIZE_WORDS];	// Buffer used to hold data until we do a write to flash

/* Only use the uint32_t data type in this structure - needs to be word aligned! */
/* The weird structure layout is to ensure we get variables spread correctly
	across FLASH memory pages such that level-wearing can be achieved. */
typedef struct _PAYLOAD_CONTROL_VARIABLES
{
	uint32_t writeCount;								// Number of times a payload slot has been written to.
	uint32_t dataValid;									// The data in this slot is current.
	uint32_t dataSize;									// The size of the data contained within the slot.	
	uint32_t pageFiller[FLASH_PAGE_SIZE_WORDS - 3];		// Ensure our four slots are FLASH page aligned (super important - dont remove!!!)
	uint32_t dataInvalid;								// The data in this slot is no longer current.
	uint32_t pageFiller2[FLASH_PAGE_SIZE_WORDS - 1];	// Ensure our four slots are FLASH page aligned (super important - dont remove!!!)
}PAYLOAD_CONTROL_VARIABLES;

/*	Application Note:
dataValid and dataInvalid are in different flash pages, therefore when a new data set is written we update the dataValid field
and the writeCount at the same time making the dataValid count higher than the invalid counter to signal that the data is valid.
Data is then invalidated by incrementing the dataInvalid field to signal that the data is not longer valid.
This is required as otherwise two writes will be required to validate and then invalidate data - this will half the life of the
flash, this is unacceptable and had to be avoided - theres probably a more elegant way of doing this to be fair?
*/

/* Only use the uint32_t data type in this structure - needs to be word aligned! */
/* This structure is the map that we are using in FLASH */
typedef struct _PAYLOAD_MEMORY_SECTION
{
	uint32_t					payloadData[NUMBER_OF_PAYLOAD_SLOTS][PAYLOAD_NUMBER_OF_PAGES][FLASH_PAGE_SIZE_WORDS];	// Payload data
	PAYLOAD_CONTROL_VARIABLES	controlVariables[NUMBER_OF_PAYLOAD_SLOTS];												// Control variables
}PAYLOAD_MEMORY_SECTION;

__attribute__((__section__(".flash_nvram")))
static PAYLOAD_MEMORY_SECTION payloadDataStore;

/* Private functions */
static uint32_t FindLowestUsedSlot(void);
static void ClearDataValidFlags(void);
static void StopTransfer(void);
static void StartTransfer(void);
static void ProcessCommandPacket(uint8_t * z_buffer);
static void ProcessDataPacket(uint8_t * z_buffer);
static void ResetTransferControlVariables(void);
static void CollatePacketToSend(uint8_t * z_data, uint8_t z_slotInUse, uint32_t z_packetToSend);
static uint16_t GetDataSizeInPackets(uint8_t z_slotInUse);
static PAYLOAD_DATA_ERROR DeterminePacketToSend(uint8_t * z_holemap, uint16_t z_totalPacketToSend, uint16_t * z_packetToSend);

uint8_t holemapBuffer[HOLEMAP_MAX_SIZE_BYTES];		
uint16_t packetToSend = 0;						// The packet number to send this time

void InitialisePayloadData(void)
{
	queueHandle = xQueueCreateStatic(PAYLOAD_DATA_QUEUE_LENGTH, PAYLOAD_DATA_QUEUE_ITEM_SIZE, &queueStorage[0][0], &queueControl);
	transferInProgress = 0;
	slotInUse = 0;
	transferCount = 0;
	pageCount = 0;
	wordCount = 0;
	justWrittenToFlash = 0;
	packetToSend = MAX_32BIT_VALUE;	// Start high so that pre-increment sets to correct value.
	RegisterThreadWithManager(PAYLOAD_DATA_THREAD);
}

QueueHandle_t GetPayloadDataQueueHandle(void)
{
	return queueHandle;
}

SemaphoreHandle_t InitialiseFlashAccessProtection(void)
{
	flashAccessSemaphore = xSemaphoreCreateRecursiveMutexStatic(&flashAccessSemaphoreStorage);
	
	return flashAccessSemaphore;
}

uint8_t PayloadDataTask(void)
{
	uint8_t buffer[PAYLOAD_DATA_QUEUE_ITEM_SIZE];
	uint8_t retVal = 0;
	
	if(uxQueueMessagesWaiting(queueHandle) != 0)
	{
		xQueueReceive(queueHandle, &buffer[0], PAYLOAD_QUEUE_TICKS_TO_WAIT);

		if(buffer[PAYLOAD_PACKET_CONTROL_INDEX] == PAYLOAD_DATA_PACKET)
		{
			ProcessDataPacket(&buffer[0]);
		}
		else if(buffer[PAYLOAD_PACKET_CONTROL_INDEX] == PAYLOAD_COMMAND_PACKET)
		{
			ProcessCommandPacket(&buffer[0]);
		}
		else
		{
			/* Something has gone wrong! */
		}
	}
	else
	{
		retVal = 1;
	}
	
	return retVal;
}

void PayloadDataThread(void)
{	
	uint8_t i = 0;
	
	while(1)
	{
		ProvideThreadHeartbeat(PAYLOAD_DATA_THREAD);
		/* Loop until we don't have an item to process. */
		for(i = 0; i < PAYLOAD_DATA_QUEUE_LENGTH; i++)
		{		
			if(PayloadDataTask())
			{
				break;
			}
		}
		
		vTaskDelay(25);
	}
}

static void ProcessDataPacket(uint8_t * z_buffer)
{
	PAYLOAD_MEMORY_SECTION * p_payloadDataStore = &payloadDataStore;
	uint32_t bufferVar = 0;

	if(z_buffer != 0)
	{
		if(transferInProgress != 0)
		{
			volatile uint32_t temp = MAXIMUM_PAYLOAD_SIZE_WORDS;
			if(transferCount < MAXIMUM_PAYLOAD_SIZE_WORDS)
			{
				/* Copy across the data portion of this packet to local storage for now */
				memcpy(&pageBuffer[wordCount], &z_buffer[PAYLOAD_PACKET_DATA_INDEX], sizeof(pageBuffer[wordCount]));
						
				/* We have had more data since we last wrote to flash. */
				justWrittenToFlash = 0;

				/* Increment our word count until we have filled a page. */
				wordCount++;
				if(wordCount >= FLASH_PAGE_SIZE_WORDS)
				{
					uint8_t i = 0;
							
					if(flashAccessSemaphore != 0)
					{
						if(xSemaphoreTakeRecursive(flashAccessSemaphore, FLASH_ACCESS_TICKS_TO_WAIT) != 0)
						{
							/* Copy across to the buffer just before we perform a write. (has to be done word by word hence no memcpy) */
							for(i = 0; i < FLASH_PAGE_SIZE_WORDS; i++)
							{
								p_payloadDataStore->payloadData[slotInUse][pageCount][i] = pageBuffer[i];
							}

							/* We have filled the page buffer so now write to flash. */
							WriteToFlash();

							/* We have just written to flash so record this */
							justWrittenToFlash = 1;

							/* Clear our page buffer for next itteration. */
							ClearPageBuffer();

							/* Reset our indices */
							wordCount = 0;
							pageCount++;

							xSemaphoreGiveRecursive(flashAccessSemaphore);
						}
						else
						{
							/* Abort the transfer. */
							ResetTransferControlVariables();
						}
					}
				}
				/* Increment our data counter. */
				transferCount++;
			}
			else
			{
				/* Abort the transfer. */
				ResetTransferControlVariables();
			}
		}
	}
}

static void ResetTransferControlVariables(void)
{
	transferInProgress = 0;
	transferCount = 0;
	pageCount = 0;	
	wordCount = 0;	
	justWrittenToFlash = 0;
	memset(&pageBuffer[0], 0, sizeof(pageBuffer));
}

static void ProcessCommandPacket(uint8_t * z_buffer)
{
	if(z_buffer != 0)
	{
		if(flashAccessSemaphore != 0)
		{
			if(xSemaphoreTakeRecursive(flashAccessSemaphore, FLASH_ACCESS_TICKS_TO_WAIT) != 0)
			{
				switch(z_buffer[PAYLOAD_PACKET_COMMAND_INDEX])
				{
					case PAYLOAD_DATA_TRANSFER_START:
						/* Start a transfer */
						StartTransfer();
						break;

					case PAYLOAD_DATA_TRANSFER_STOP:
						/* Stop the current transfer */
						StopTransfer();
						break;
				}
				xSemaphoreGiveRecursive(flashAccessSemaphore);
			}
			else
			{
				/* Abort the transfer. */
				ResetTransferControlVariables();
			}
		}
	}
}

static void StartTransfer(void)
{
	/* Beginning a new transfer so clean up anything we have lying around from the last transfer. */
	ResetTransferControlVariables();

	/* Find the slot that we want to use for this transfer */
	slotInUse = FindLowestUsedSlot();

	/* Clear the holemap at this point */
	ClearHoleMap();

	/* Mark all current transfers as no longer being valid. */
	ClearDataValidFlags();

	/* Clear our page buffer for next itteration. */
	ClearPageBuffer();

	/* Record that we are now performing a transfer */
	transferInProgress = 1;
	justWrittenToFlash = 0;
}

static void StopTransfer(void)
{
	PAYLOAD_MEMORY_SECTION * p_payloadDataStore = &payloadDataStore;

	/* Ignore command if we arent doing anything. */
	if(transferInProgress == 1)
	{
		/* Need at least one byte for it to be considered valid data. */
		if(transferCount != 0)
		{
			/* We dont want to write to flash again if have already done this for this data set. */
			if(justWrittenToFlash == 0)
			{
				uint8_t i = 0;

				/* Copy across to the buffer just before we perform a write. (has to be done word by word hence no memcpy) */
				for(i = 0; i < FLASH_PAGE_SIZE_WORDS; i++)
				{
					p_payloadDataStore->payloadData[slotInUse][pageCount][i] = pageBuffer[i];
				}

				/* We have received a stop command so we need to write what we have to flash. */
				WriteToFlash();
			}

			/* Need to store these variables now as they are going to be wiped from flash. */
			uint32_t writeCount = p_payloadDataStore->controlVariables[slotInUse].writeCount + 1;	
			uint32_t dataValid = p_payloadDataStore->controlVariables[slotInUse].dataValid + 1;		

			/* Clear the page buffer. */
			ClearPageBuffer();

			/* Write to the correct flash location to update the page number field internally */
			p_payloadDataStore->controlVariables[slotInUse].dataSize = 1;

			/* Clear the page we have stored dataInvalid in */
			ClearPage();
			p_payloadDataStore->controlVariables[slotInUse].writeCount = writeCount;
			p_payloadDataStore->controlVariables[slotInUse].dataValid = dataValid;
			p_payloadDataStore->controlVariables[slotInUse].dataSize = transferCount;
			WriteToFlash();
			justWrittenToFlash = 0;
			transferInProgress = 0;
			transferCount = 0;
			pageCount = 0;	
			wordCount = 0;	
		}
		else
		{
			ResetTransferControlVariables();
		}
	}
}

static uint32_t FindLowestUsedSlot(void)
{
	PAYLOAD_MEMORY_SECTION * p_payloadDataStore = &payloadDataStore;
	uint8_t i = 0;
	uint32_t lowestUsage = 0xFFFFFFFF;	// Assume maximum usage to start
	uint32_t slotToUse = 0;

	/* Find the lowest used slot */
	for(i = 0; i < NUMBER_OF_PAYLOAD_SLOTS; i++)
	{
		if(p_payloadDataStore->controlVariables[i].writeCount < lowestUsage)
		{
			lowestUsage = p_payloadDataStore->controlVariables[i].writeCount;
			slotToUse = i;
		}
	}
	return slotToUse;
}

static void ClearDataValidFlags(void)
{
	PAYLOAD_MEMORY_SECTION * p_payloadDataStore = &payloadDataStore;
	uint8_t i = 0;

	for(i = 0; i < NUMBER_OF_PAYLOAD_SLOTS; i++)
	{
		/* If our data in a given slot is currently valid (valid counter > invalid counter */
		if(p_payloadDataStore->controlVariables[i].dataValid > p_payloadDataStore->controlVariables[i].dataInvalid)
		{
			uint32_t j = 0;

			/* Clear the page buffer. */
			ClearPageBuffer();
			/* Write to the correct flash location to update the page number field internally */
			p_payloadDataStore->controlVariables[i].dataInvalid = 0;
			/* Clear the page we have stored dataInvalid in */
			ClearPage();
			p_payloadDataStore->controlVariables[i].dataInvalid = p_payloadDataStore->controlVariables[i].dataValid;
			/* Write the page buffer into FLASH. */
			WriteToFlash();

			/* Clear all of the flash pages for the currently used slot. */
			for(j = 0; j < PAYLOAD_NUMBER_OF_PAGES; j++)
			{
				p_payloadDataStore->payloadData[i][j][0] = 0;
				ClearPage();
			}
		}
	}
}

 /* This function will not do anything unless we have just had a download,
    in which case it will initialise the payload flash to all zeros.  */
void InitialiseFlash(void)
{
	PAYLOAD_MEMORY_SECTION * p_payloadDataStore = &payloadDataStore;
	
	/* If these values are present then we have just had a fresh download. */
	if(p_payloadDataStore->controlVariables[0].dataValid == FLASH_LOCATION_UNINITIALISED &&
	   p_payloadDataStore->controlVariables[0].dataInvalid == FLASH_LOCATION_UNINITIALISED)
	{
		/* Wipe all of the payload flash control variables. */
		ClearPayloadFlash();
	}	
}

/* This function will clear the payload data flash storage */
void ClearPayloadFlash(void)
{
	PAYLOAD_MEMORY_SECTION * p_payloadDataStore = &payloadDataStore;
	uint32_t i = 0;
	
	/* We don't need to lock during initialisation as the scheduler isn't running but for debug flash clear we do */
	if((xSemaphoreTakeRecursive(flashAccessSemaphore, FLASH_ACCESS_TICKS_TO_WAIT) != 0))
	{
		/* Clear all control variable pages. */
		for(i = 0; i < NUMBER_OF_PAYLOAD_SLOTS; i++)
		{
			/* Clear the page buffer. */
			ClearPageBuffer();
			/* Write to the correct flash location to update the page number field internally */
			p_payloadDataStore->controlVariables[i].writeCount = 0;

			/* Clear the page we have stored dataInvalid in */
			ClearPage();
			p_payloadDataStore->controlVariables[i].writeCount = 0;
			p_payloadDataStore->controlVariables[i].dataValid = 0;
			p_payloadDataStore->controlVariables[i].dataSize = 0;
			/* Write the page buffer into FLASH. */
			WriteToFlash();
		}
	
		/* Clear all invalidate data pages. */
		for(i = 0; i < NUMBER_OF_PAYLOAD_SLOTS; i++)
		{
			/* Clear the page buffer. */
			ClearPageBuffer();
			/* Write to the correct flash location to update the page number field internally */
			p_payloadDataStore->controlVariables[i].dataInvalid = 0;

			/* Clear the page we have stored dataInvalid in */
			ClearPage();
			p_payloadDataStore->controlVariables[i].dataInvalid = 0;
			/* Write the page buffer into FLASH. */
			WriteToFlash();
		}
		/* Return the semaphore */
		xSemaphoreGiveRecursive(flashAccessSemaphore);			
	}
}

uint8_t PayloadDataPresent(void)
{
	PAYLOAD_MEMORY_SECTION * p_payloadDataStore = &payloadDataStore;
	uint16_t i = 0;
	uint8_t retVal = 0;
	
	if(flashAccessSemaphore != 0)
	{
		if(xSemaphoreTakeRecursive(flashAccessSemaphore, FLASH_ACCESS_TICKS_TO_WAIT) != 0)
		{
			/* Find the active slot */
			for(i = 0; i < NUMBER_OF_PAYLOAD_SLOTS; i++)
			{
				if(p_payloadDataStore->controlVariables[i].dataValid > p_payloadDataStore->controlVariables[i].dataInvalid)
				{
					retVal = 1;
					break;
				}
			}	
			xSemaphoreGiveRecursive(flashAccessSemaphore);
		}
	}
	return retVal;
}

PAYLOAD_DATA_ERROR GetPayloadData(uint8_t * z_data, uint16_t z_size, uint16_t * z_packetNumber)
{
	PAYLOAD_MEMORY_SECTION * p_payloadDataStore = &payloadDataStore;
	PAYLOAD_DATA_ERROR retVal = NO_ERROR_PAYLOAD_DATA;
	uint8_t slotInUseRead = NUMBER_OF_PAYLOAD_SLOTS;	// Set to be higher than max allowable slot number for now (with zero index).
	uint16_t totalPacketsToSend;

	if((z_data != 0) && (z_size >= PAYLOAD_DATA_PACKET_SIZE) && (z_packetNumber != 0))
	{
		if(flashAccessSemaphore != 0)
		{
			if(xSemaphoreTakeRecursive(flashAccessSemaphore, FLASH_ACCESS_TICKS_TO_WAIT) != 0)
			{
				uint16_t i = 0;

				/* Find the active slot */
				for(i = 0; i < NUMBER_OF_PAYLOAD_SLOTS; i++)
				{
					if(p_payloadDataStore->controlVariables[i].dataValid > p_payloadDataStore->controlVariables[i].dataInvalid)
					{
						slotInUseRead = i;
						break;
					}
				}

				/* Check if we found a valid slot */
				if(slotInUseRead < NUMBER_OF_PAYLOAD_SLOTS)
				{
					GetHolemap(&holemapBuffer[0], sizeof(holemapBuffer));
					totalPacketsToSend = GetDataSizeInPackets(slotInUseRead);

					if(DeterminePacketToSend(&holemapBuffer[0], totalPacketsToSend, &packetToSend) != NO_DATA_READY)
					{
						CollatePacketToSend(z_data, slotInUseRead, packetToSend);									
						retVal = NO_ERROR_PAYLOAD_DATA;
						*z_packetNumber = packetToSend;
					}
					else
					{
						/* We have completed the holemap - invalidate all of the data present. */
						ClearDataValidFlags();

						retVal = NO_DATA_READY;
					}				
				}
				else
				{
					retVal = NOT_PROCESSING_PAYLOAD_DATA;
				}

				xSemaphoreGiveRecursive(flashAccessSemaphore);
			}
			else
			{
				retVal = SEMAPHORE_TIMED_OUT;
			}
		}
	}
	else
	{
		retVal = INVALID_INPUT;
	}
	return retVal;
}

static PAYLOAD_DATA_ERROR DeterminePacketToSend(uint8_t * z_holemap, uint16_t z_totalPacketToSend, uint16_t * z_packetToSend)
{
	PAYLOAD_DATA_ERROR retVal = NO_DATA_READY;
	uint16_t i = 0;

	if(z_holemap != 0 && z_packetToSend != 0)
	{
		(*z_packetToSend)++;

		/* Determine our bit / byte indexs for the end of our holemap */
		uint8_t endOfHoleMapBits = z_totalPacketToSend % BITS_IN_A_BYTE;
		uint8_t endOfHoleMapBytes = (z_totalPacketToSend - endOfHoleMapBits) / BITS_IN_A_BYTE;

		/* Determine our starting index into the holemap. */
		uint8_t bitCountLocal = *z_packetToSend % BITS_IN_A_BYTE;
		uint8_t byteCountLocal = (*z_packetToSend - bitCountLocal) / BITS_IN_A_BYTE;

		for(i = 0; i < z_totalPacketToSend; i++)
		{
			/* If we have found a hole. */
			if(!(z_holemap[byteCountLocal] & (1 << bitCountLocal)))
			{
				*z_packetToSend = (byteCountLocal * BITS_IN_A_BYTE) + bitCountLocal;
				retVal = NO_ERROR_PAYLOAD_DATA;
				break;
			}

			/* Increment our bit and byte indexs */
			bitCountLocal++;
			if(bitCountLocal >= BITS_IN_A_BYTE)
			{
				byteCountLocal++;
				bitCountLocal = 0;
			}

			/* If we reach the end of our holemap then loop back around. */
			if((byteCountLocal == endOfHoleMapBytes) && (bitCountLocal == endOfHoleMapBits))
			{
				byteCountLocal = 0;
				bitCountLocal = 0;
			}
		}

		/* Limit our loop to the amount of data that we have. (Round up to nearest packet size boundary) */
		if(*z_packetToSend >= z_totalPacketToSend)
		{
			*z_packetToSend = 0;
		}
		
	}
	return retVal;
}

static uint16_t GetDataSizeInPackets(uint8_t z_slotInUse)
{
	PAYLOAD_MEMORY_SECTION * p_payloadDataStore = &payloadDataStore;
	uint32_t dataSizeWords = p_payloadDataStore->controlVariables[z_slotInUse].dataSize;
	uint32_t wordsOverAPacket = (dataSizeWords % PACKET_SIZE_IN_WORDS);
	uint32_t wordsToAdd = (wordsOverAPacket == 0) ? 0 : (PACKET_SIZE_IN_WORDS - wordsOverAPacket);
	uint32_t dataSizeInWordsRoundedToPacket = dataSizeWords + wordsToAdd;
	uint32_t dataSizeInPackets = dataSizeInWordsRoundedToPacket / PACKET_SIZE_IN_WORDS;

	return dataSizeInPackets;
}

static void CollatePacketToSend(uint8_t * z_data, uint8_t z_slotInUse, uint32_t z_packetToSend)
{
	PAYLOAD_MEMORY_SECTION * p_payloadDataStore = &payloadDataStore;
	uint32_t dataSizeWords;
	uint32_t bytesIntoBuffer;
	uint32_t bytesIntoWord;
	uint32_t wordsIntoBuffer;
	uint32_t wordsIntoPage;
	uint32_t pagesIntoBuffer;
	uint32_t wordCountRead;
	uint32_t pageCountRead;
	uint32_t dataSizePages;
	uint32_t dataWordsOverAPage;
	uint32_t i;

	/* Determine our data size in flash pages. */
	dataSizeWords = p_payloadDataStore->controlVariables[z_slotInUse].dataSize;
	dataWordsOverAPage = dataSizeWords % FLASH_PAGE_SIZE_WORDS;
	dataSizePages = dataSizeWords / FLASH_PAGE_SIZE_WORDS;

	/* Calculate the start number of bytes to send from .*/
	bytesIntoBuffer = z_packetToSend * PAYLOAD_DATA_PACKET_SIZE;
	wordsIntoBuffer = bytesIntoBuffer / sizeof(uint32_t);

	/* Calculate where we need to start sending from in the flash buffer. */
	wordsIntoPage =  wordsIntoBuffer % FLASH_PAGE_SIZE_WORDS;
	pagesIntoBuffer = wordsIntoBuffer / FLASH_PAGE_SIZE_WORDS;

	/* Set up our counters */
	wordCountRead = wordsIntoPage;
	pageCountRead = pagesIntoBuffer;

	/* Loop for each word element of our packet. */
	for(i = 0; i < PACKET_SIZE_IN_WORDS; i++)
	{
		/* Copy out a word at a time. */
		memcpy(&z_data[i * sizeof(uint32_t)], &p_payloadDataStore->payloadData[z_slotInUse][pageCountRead][wordCountRead], sizeof(uint32_t));
		wordCountRead++;

		/* We have reached the end of this flash page so loop */
		if(wordCountRead >= FLASH_PAGE_SIZE_WORDS)
		{
			wordCountRead = 0;
			pageCountRead++;
		}

		/* We have reached the limit of our data so exit - this is expected to happen at the end of our data. */
		if((pageCountRead >= dataSizePages) && (wordCountRead >= dataWordsOverAPage))
		{
			break;
		}
	}
}