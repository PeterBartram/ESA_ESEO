#include "SatelliteOperations.h"
#include "WOD.h"

/* Configuration values - don't change these! */
#define NUMBER_OF_WOD_SAMPLES_PER_ORBIT	100
#define BYTES_PER_WOD_SAMPLE			40
#define NUMBER_OF_WOD_PER_PACKET		5
#define NUMBER_OF_WOD_PACKETS			20
#define WOD_SAMPLE_PERIOD				60000
#define SEQUENCE_NUMBER_UPDATE_PERIOD	120000
#define	WOD_SEMAPHORE_TICKS_TO_WAIT		5
#define TICKS_TO_WAIT_QUEUE				5

/* Memory used for storing the WOD. */
static uint8_t wodArray[NUMBER_OF_WOD_SAMPLES_PER_ORBIT][BYTES_PER_WOD_SAMPLE];

/* WOD value update time memory */
static void * timerID = 0;
static StaticTimer_t timerBuffer;
static SemaphoreHandle_t wodAccessSemaphore;
static StaticSemaphore_t wodAccessSemaphoreStorage;
static uint8_t wodIndex = 0;					// The current location we are in the WOD buffer.
static uint8_t wodUpdate = 0;					//	Indicate that we should update the wod data.
static void WodTakeSample(xTimerHandle z_timer);
static void SampleWODData(uint32_t wodIndex);


/* Sequence number timer update */
static void * sequenceNumberTimerID = 0;
static StaticTimer_t sequenceNumberTimerBuffer;
static uint8_t sequenceNumberUpdate = 0;
static void SampleSequenceNumber(xTimerHandle z_timer);
void UpdateSequenceNumber(void);
static QueueHandle_t canQueue = 0;

static xTimerHandle wodSampleTimer = 0;

/* Initialise whole orbit data sampling. */
void InitialiseWOD(void)
{
	memset(&wodArray[0], 0, sizeof(wodArray));

	/* Set up a timer to sample the orbit data once a minute. */
	wodSampleTimer = xTimerCreateStatic("WOD", WOD_SAMPLE_PERIOD, AUTO_RELOAD, &timerID, &WodTakeSample, &timerBuffer);
	xTimerStart(wodSampleTimer, 0);

	/* Create a timer for every 2 minutes to update the sequence number */
	wodSampleTimer = xTimerCreateStatic("SequeneNo", SEQUENCE_NUMBER_UPDATE_PERIOD, AUTO_RELOAD, &sequenceNumberTimerID, &SampleSequenceNumber, &sequenceNumberTimerBuffer);
	xTimerStart(wodSampleTimer, 0);

	/* Fetch the command queue to post to the CAN OD */
	canQueue = GetCommandQueueHandle();

	/* Create a semaphore to protect access to the WOD data store. */
	wodAccessSemaphore = xSemaphoreCreateRecursiveMutexStatic(&wodAccessSemaphoreStorage);
	wodIndex = 0;
	wodUpdate = 0;
	sequenceNumberUpdate = 0;
}

/* Update WOD data if necessary */
void UpdateWODData(void)
{
	if(wodUpdate != 0)
	{
		wodUpdate = 0;

		/* Take a WOD sample. */
		SampleWODData(wodIndex);

		/* Update our WOD sample counter and reset if we reach the end of the buffer. */
		wodIndex++;
		wodIndex = (wodIndex >= NUMBER_OF_WOD_SAMPLES_PER_ORBIT) ? 0 : wodIndex;
	}	
}

/* Take a single WOD sample and add it to the RAM buffer. */
static void SampleWODData(uint32_t z_wodIndex)
{
	memset(&wodArray[z_wodIndex][0], 0, BYTES_PER_WOD_SAMPLE);
	uint32_t data;
	uint32_t data1;
	uint32_t data2;

	BITSTREAM_CONTROL bitStream;

	/* Initialise our bitstream */
	bitStream.bitInput = 0;
	bitStream.byteInput = 0;

	if(wodAccessSemaphore != 0)
	{
		/* Take the semaphore. */
		xSemaphoreTakeRecursive(wodAccessSemaphore, WOD_SEMAPHORE_TICKS_TO_WAIT);

		/* Fetch the required data and add it to the bit stream */
		/* EPS */
		GetCANObjectDataRaw(AMS_EPS_CCT_T, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_EPS_ENC_T, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_EPS_DCDC_T, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_EPS_DCDC_I, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_EPS_DCDC_V, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_EPS_6V_V, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_EPS_9V_V, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_EPS_3V3_V, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_EPS_6V_I, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_EPS_3V3_I, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_EPS_9V_I, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		
		/* AMSAT Receiver */
		GetCANObjectDataRaw(AMS_L_RSSI_TRANS, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_L_DOPPLER_COMMAND, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_L_RSSI_COMMAND, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_L_T, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		
		/* AMSAT Transmitter */
		GetCANObjectDataRaw(AMS_VHF_BPSK_PA_T, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_VHF_FP, &data);
		AddToBitStream(&bitStream, &wodArray[wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_VHF_RP, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_VHF_BPSK_PA_I, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_VHF_FM_PA_I, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_VHF_FM_PA_T, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		
		/* AMSAT CCT */
		GetCANObjectDataRaw(AMS_OBC_CMD_WDT_TIME_REM, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, 0);
		GetCANObjectDataRaw(AMS_OBC_P_UP, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 4, data);
		GetCANObjectDataRaw(AMS_OBC_MEM_STAT_1, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(AMS_OBC_CAN_STATUS, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 13, data);
		GetCANObjectDataRaw(AMS_OBC_CAN_PL, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 4, data);
		GetCANObjectDataRaw(AMS_OBC_CAN_PL_STATUS, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 13, data);
		
		/* ESEO CAN (EPS) */
		GetCANObjectDataRaw(PMM_TEMP_SP1_SENS_1, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(PMM_TEMP_SP2_SENS_1, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(PMM_TEMP_SP3_SENS_1, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);
		GetCANObjectDataRaw(PMM_CURRENT_BP1, &data);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 8, data);		
		GetCANObjectDataRaw(PMM_VOLTAGE_MB, &data);			
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 16, data);

		/* ESEO AOCS data */
		GetCANObjectDataRaw(ACS_OMEGA_P, &data);
		GetCANObjectDataRaw(ACS_OMEGA_Q, &data1);
		GetCANObjectDataRaw(ACS_OMEGA_R, &data2);
		/* Calculate magnitude of angular velocity */
		data = sqrt(data*data+data1*data1+data2*data2);
		AddToBitStream(&bitStream, &wodArray[z_wodIndex][0], 32, data);
		
		/* Return our semaphore. */
		xSemaphoreGiveRecursive(wodAccessSemaphore);
	}
}

/* GetWODData fetches a bit streamed buffer of the five WODs that make up a
   WOD payload (~200 bytes), this is returned in z_wodDataBuffer.
   the return value from this function is the sequence number to send. */
uint32_t GetWODData(uint8_t z_wodDataNumber, uint8_t * z_wodDataBuffer)
{
	if(z_wodDataBuffer != 0)
	{
		uint32_t i = 0;
		volatile uint32_t wodSample = 0;
		int32_t wodSampleCalculation = 0;
		
		/* Perform limit checking on max and min bounds - we are indexed from 1! */
		if(z_wodDataNumber > NUMBER_OF_WOD_PACKETS)
		{
			z_wodDataNumber = NUMBER_OF_WOD_PACKETS;
		}
		else if(z_wodDataNumber == 0)
		{
			z_wodDataNumber = 1;
		}

		/* Calculate the starting position */
		wodSampleCalculation = wodIndex + ((z_wodDataNumber - 1) * NUMBER_OF_WOD_PER_PACKET);

		/* Perform the wrap around if required */
		wodSample = wodSampleCalculation % NUMBER_OF_WOD_SAMPLES_PER_ORBIT;
 
		if(wodAccessSemaphore != 0)
		{
			xSemaphoreTakeRecursive(wodAccessSemaphore, WOD_SEMAPHORE_TICKS_TO_WAIT);
			/* Loop for the number of WODs we want to send per packet. */
			for(i = 0; i < NUMBER_OF_WOD_PER_PACKET; i++)
			{				
				/* Update the output buffer with the WOD data. */
				memcpy(&z_wodDataBuffer[i * BYTES_PER_WOD_SAMPLE], &wodArray[wodSample][0], BYTES_PER_WOD_SAMPLE);

				wodSample++;
				wodSample = wodSample % NUMBER_OF_WOD_SAMPLES_PER_ORBIT;
			}
			xSemaphoreGiveRecursive(wodAccessSemaphore);
		}
	}
	return 0;
}

/* Callback function for the timer that will tell us to take a WOD sample. */
static void WodTakeSample(xTimerHandle z_timer)
{
	wodUpdate = 1;
}

/* This function will check if the sequence number timer has expired 
   and if it has then it will update the sequence number is the OD. */
void UpdateSequenceNumber(void)
{
	if(sequenceNumberUpdate	 != 0)
	{
		uint8_t buffer[CAN_QUEUE_ITEM_SIZE];
		uint32_t data = 0;
		sequenceNumberUpdate = 0;

		if(canQueue != 0)
		{
			GetCANObjectDataRaw(AMS_OBC_SEQNUM, &data);
			data++;
			/* Clear out output buffer */
			memset(buffer, 0, sizeof(buffer));		
			/* Indicate we are an internal source */
			buffer[PACKET_SOURCE_INDEX] = ON_TARGET_DATA;
			/* 4 bytes of data + one for the CAN ID */
			buffer[PACKET_SIZE_INDEX] = 5;
			/* Add the CAN ID. */
			buffer[OBJECT_DICTIONARY_ID_INDEX] = AMS_OBC_SEQNUM;
			/* Copy in our data. */
			memcpy(&buffer[PACKET_DATA_INDEX], &data, sizeof(data));
			/* Send the data over the queue. */
			xQueueSend(canQueue, &buffer, TICKS_TO_WAIT_QUEUE);
		}
	}
}

/* Callback function from the timer, just sets a flag to tell 
   the satellite operations thread to increase the sequence number. */
static void SampleSequenceNumber(xTimerHandle z_timer)
{
	sequenceNumberUpdate = 1;
}

