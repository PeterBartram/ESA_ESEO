
#include "FreeRTOS.h"
#include "task.h"
#include "Downlink.h"
#include "DACout.h"
#include "FECencoder.h"
#include "CANODInterface.h"
#include "CANObjectDictionary.h"
#include <string.h>
#include "BitStream.h"
#include "SatelliteOperations.h"
#include "PayloadData.h"
#include "timers.h"
#include "Watchdog.h"

/* Don't change any of these values! */
#define NUMBER_OF_PREPERATION_BUFFERS 2		
#define SIZE_OF_PREPERATION_BUFFERS	650		
#define NUMBER_OF_FRAME_TYPES	24			
#define FITTER_MESSAGE_SIZE		200			
#define WOD_SIZE				200
#define NUMBER_OF_PACKETS_IN_SEQUENCE	4	
#define SEQUENCE_SEND_TELEMETRY 0			
#define START_OF_PAYLOAD_DATA_SECTION	56	
#define START_OF_PAYLOAD_PACKET_FRAME_ID_SECTION 2
#define START_OF_PAYLOAD_PACKET_DATA_SECTION 4
#define SIZE_OF_PAYLOAD_DATA_PACKET_SECTION 252

#define PAYLOAD_DATA_FRAME	31

/* Fitter message IDs. */
#define FITTER_MESSAGE_1	1
#define FITTER_MESSAGE_2	7
#define FITTER_MESSAGE_3	13
#define FITTER_MESSAGE_4	19

#define FITTER_SLOT_0	0
#define FITTER_SLOT_1	1
#define FITTER_SLOT_2	2
#define FITTER_SLOT_3	3

static uint8_t sendBuffer[NUMBER_OF_PREPERATION_BUFFERS][SIZE_OF_PREPERATION_BUFFERS];	//Two buffers that are altenately filled with data to be transmitted.
static uint8_t sendBufferIndex = 0;														// Which buffer are we currently using.			
static DATA_MODE dataMode = MODE_RX_ONLY;												// The current data mode. 
static DATA_MODE dataModeNew = MODE_RX_ONLY;											// Commanded data mode to switch into.
static uint8_t rttPacketNumber = 0;														// The current RTT packet number - either one of zero and alternating.
static uint8_t frameNumber = 1;															// The current frame number
static uint8_t packetSequenceNumber = 0;												// Which packet number we are currently sending when in payload mode. (0 = telemetry 1,2,3 = science data)
static uint8_t fitterBuffer[FITTER_MESSAGE_SIZE];												// Buffer used by several functions - here to save space on the stack.
static uint32_t packetsDownlinked = 0;													// Number of packets we have donwlinked.

static void Operate1k2Mode(uint8_t z_rttPacketNumber, uint8_t z_frameNumber);
static void Operate4k8Mode(uint8_t z_rttPacketNumber, uint8_t z_frameNumber);
static void Operate4k8PayloadMode(uint8_t z_rttPacketNumber, uint8_t z_frameNumber, uint8_t z_packetSequenceNumber);
static void ActionChangeDownlinkMode(void);
static void AddSatelliteIDBytes(BITSTREAM_CONTROL * z_bitStream, uint8_t * z_buffer, uint8_t z_frameNumber);
static uint32_t FetchTelemetryData(BITSTREAM_CONTROL * z_bitStream, uint8_t z_rttNumber, uint8_t * z_buffer);
static uint32_t FetchFrameSpecificData(BITSTREAM_CONTROL * z_bitStream, uint8_t z_frameNumber, uint8_t * z_buffer);
static void FetchFitterMessageFrameData(BITSTREAM_CONTROL * z_bitStream, uint8_t z_frameNumber, uint8_t * z_buffer);
static void FetchPayloadDataFrameData(BITSTREAM_CONTROL * z_bitStream, uint8_t z_frameNumber, uint8_t * z_buffer);
static void AddCommonTelemetrySection(BITSTREAM_CONTROL * z_control, uint8_t * z_buffer);
static void AddRTTTelemetrySection1(BITSTREAM_CONTROL * z_control, uint8_t * z_buffer);
static void AddRTTTelemetrySection2(BITSTREAM_CONTROL * z_control, uint8_t * z_buffer);
static void UpdatePacketCounters(void);

/* Counter update functionality */
static StaticTimer_t packetCounterUpdateTimer;
static xTimerHandle  packetCounterUpdateTimerHandle;
#define DELAY_1S 1000
#define AUTO_RELOAD 1
static void packetCounterUpdateTimerCallback(xTimerHandle xTimer);
static uint8_t packetCounterUpdate = 0;
static QueueHandle_t commandQueueHandle = 0;

void InitialiseDownlink(void)
{
	/* Initialise the FEC block. */
	InitFEC();
	/* Initialise the DAC */
	InitialiseDAC();

	/* Reset variables. */
	sendBufferIndex = 0;
	dataMode = MODE_1K2;
	dataModeNew = MODE_1K2;
	rttPacketNumber = 0;
	frameNumber = 1;
	packetSequenceNumber = 0;
	packetsDownlinked = 0;
	
	memset(sendBuffer, 0, sizeof(sendBuffer));
	
	/* Set up a timer to handle updating the CAN OD with packet counts etc etc. */
	packetCounterUpdateTimerHandle = xTimerCreateStatic("DownlinkPacketUpdate", DELAY_1S, AUTO_RELOAD, 0, &packetCounterUpdateTimerCallback, &packetCounterUpdateTimer);
	xTimerStart(packetCounterUpdateTimerHandle, 0);	
	/* Need to be able to update packet counters etc etc. */
	commandQueueHandle =  GetCommandQueueHandle();
	RegisterThreadWithManager(DOWNLINK_THREAD);
}

void DownlinkThread(void)
{
	while(1)
	{
		ProvideThreadHeartbeat(DOWNLINK_THREAD);
		DownlinkTask();
		UpdatePacketCounters();
		vTaskDelay(10);
	}
}

/* This is the main task for handling downlinking of data - it operates based upon
   the downling mode that it has been placed into and will collect all of the
   appropriate telemetry from the OD and package everything up to be sent. */
void DownlinkTask(void)
{
	uint32_t data = 0;

	/* Don't do anything unless the DAC is ready for us. */
	if(TransmitReady() != 0)
	{
		switch(dataMode)
		{
			case MODE_RX_ONLY:
			{
				break;
			}
			case MODE_1K2:
			{
				Operate1k2Mode(rttPacketNumber, frameNumber);
				packetsDownlinked++;
				break;
			}
			case MODE_4K8:
			{
				Operate4k8Mode(rttPacketNumber, frameNumber);
				packetsDownlinked++;
				break;
			}
			case MODE_4K8_PAYLOAD:
			{
				/* Increment our packet sequence number and limit to the number of packets in the sequence. */
				packetSequenceNumber++;
				packetSequenceNumber = (packetSequenceNumber >= NUMBER_OF_PACKETS_IN_SEQUENCE) ? 0 : packetSequenceNumber;

				Operate4k8PayloadMode(rttPacketNumber, frameNumber, packetSequenceNumber);
				packetsDownlinked++;

				break;
			}
			default:
			{
				/* Something screwy has happened - try to resolve */
				sendBufferIndex = 0;
				break;
			}
		}

		/* We only want to increment the RTT counts when we send an RTT packet. */
		if(packetSequenceNumber == 0)
		{
			/* Loop from 1-24 repeatedly. */
			frameNumber++;
			frameNumber = (frameNumber > NUMBER_OF_FRAME_TYPES) ? 1 : frameNumber;

			/* Flip-flop the RTT packet counter so we can alternate what we send down. */
			rttPacketNumber = (rttPacketNumber >= 1) ? 0 : 1;
		}
	}
}

/* Main function for generating the 1k2 telemetry packets to be downlinked. 
  z_rttPacketNumber should flip/flop between 0 and 1 such that we get alternating 
  packets with the full range of telemetry within them.
  z_frameNumber counts from 1-24 and tells which frame should be transmitted. */
static void Operate1k2Mode(uint8_t z_rttPacketNumber, uint8_t z_frameNumber)
{
	uint16_t count = 0;
	uint8_t * outputFEC = 0;
	BITSTREAM_CONTROL bitStream;	// Create a bit stream

	/* Initialise the bit stream. */
	memset(&bitStream, 0 , sizeof(bitStream));

	/* Clear our output buffer. */
	memset(sendBuffer, 0, sizeof(sendBuffer));

	/* Add the SAT ID and frame type data to the packet first. */
	AddSatelliteIDBytes(&bitStream, &sendBuffer[0][0], z_frameNumber);

	/* Fetch all telemetry data from the OD */
	FetchTelemetryData(&bitStream, z_rttPacketNumber, &sendBuffer[0][0]);

	/* Get the specific data required for this packet. */
	FetchFrameSpecificData(&bitStream, z_frameNumber, &sendBuffer[0][0]);

	/* We are in 1k2 mode so we simply need to get data and FEC it ready for TX */
	outputFEC = EncodeFEC(&sendBuffer[0][0], &count);
	if(outputFEC != 0)
	{
		memcpy(&sendBuffer[0][0], outputFEC, count);
		DACOutputBufferReady(&sendBuffer[0][0], count);
	}
	else
	{
		/* Error has occurred */
	}
}

/* Main function for generating the 4k8 telemetry packets to be downlinked. 
  z_rttPacketNumber should flip/flop between 0 and 1 such that we get alternating 
  packets with the full range of telemetry within them.
  z_frameNumber counts from 1-24 and tells which frame should be transmitted.
  This function relies on the fact that the first packet has already been
  encoded when we swicthed modes and placed in sendBuffer in the appropriate place. */
static void Operate4k8Mode(uint8_t z_rttPacketNumber, uint8_t z_frameNumber)
{
	uint16_t count = 0;
	uint8_t * outputFEC = 0;
	BITSTREAM_CONTROL bitStream;	// Create a bit stream

	if(sendBufferIndex < NUMBER_OF_PREPERATION_BUFFERS)
	{
		/* Send the data that we prepared last time to the DAC 
		(this is why the send buffer index is inverted). */
		DACOutputBufferReady(&sendBuffer[(~sendBufferIndex) & 0x1][0], SIZE_OF_PREPERATION_BUFFERS);

		/* Check we havent had any SEUs */
		sendBufferIndex &= 0x01;

		/* Initialise the bit stream. */
		memset(&bitStream, 0 , sizeof(bitStream));

		/* Clear the output buffer section we arent currently transmitting. */
		memset(&sendBuffer[sendBufferIndex][0], 0, (sizeof(sendBuffer) / NUMBER_OF_PREPERATION_BUFFERS));

		/* Add the SAT ID and frame type data to the packet first. */
		AddSatelliteIDBytes(&bitStream, &sendBuffer[sendBufferIndex][0], z_frameNumber);

		/* Fetch all telemetry data from the OD */
		FetchTelemetryData(&bitStream, z_rttPacketNumber, &sendBuffer[sendBufferIndex][0]);

		/* Get the specific data required for this packet. */
		FetchFrameSpecificData(&bitStream, z_frameNumber, &sendBuffer[sendBufferIndex][0]);

		/* We are in 1k2 mode so we simply need to get data and FEC it ready for TX */
		outputFEC = EncodeFEC(&sendBuffer[sendBufferIndex][0], &count);

		/* Copy our FECed output into the correct place to be sent. */
		memcpy(&sendBuffer[sendBufferIndex][0], outputFEC, count);

		/* Flip to the other buffer for next cycle. */
		sendBufferIndex = (sendBufferIndex >= 1) ? 0 : 1;
	}
	else
	{
		/* An error has occurred. */
	}
}

/* Main function for generating the 4k8 telemetry packets to be downlinked. 
  z_rttPacketNumber should flip/flop between 0 and 1 such that we get alternating 
  packets with the full range of telemetry within them.
  z_frameNumber counts from 1-24 and tells which frame should be transmitted.
  This function relies on the fact that the first packet has already been
  encoded when we swicthed modes and placed in sendBuffer in the appropriate place. */
static void Operate4k8PayloadMode(uint8_t z_rttPacketNumber, uint8_t z_frameNumber, uint8_t z_packetSequenceNumber)
{
	uint16_t count = 0;
	uint8_t * outputFEC = 0;
	BITSTREAM_CONTROL bitStream;	// Create a bit stream

	if(sendBufferIndex < NUMBER_OF_PREPERATION_BUFFERS)
	{
		/* Send the data that we prepared last time to the DAC 
		(this is why the send buffer index is inverted). */
		DACOutputBufferReady(&sendBuffer[(~sendBufferIndex) & 0x1][0], 320);

		/* Check we havent had any SEUs */
		sendBufferIndex &= 0x01;

		/* Initialise the bit stream. */
		memset(&bitStream, 0 , sizeof(bitStream));

		/* Clear the output buffer section we arent currently transmitting. */
		memset(&sendBuffer[sendBufferIndex][0], 0, (sizeof(sendBuffer) / NUMBER_OF_PREPERATION_BUFFERS));

		if(z_packetSequenceNumber == SEQUENCE_SEND_TELEMETRY)
		{
			/* Add the SAT ID and frame type data to the packet first. */
			AddSatelliteIDBytes(&bitStream, &sendBuffer[sendBufferIndex][0], z_frameNumber);

			/* Fetch all telemetry data from the OD */
			FetchTelemetryData(&bitStream, z_rttPacketNumber, &sendBuffer[sendBufferIndex][0]);

			/* Get the specific data required for this packet. */
			FetchFrameSpecificData(&bitStream, z_frameNumber, &sendBuffer[sendBufferIndex][0]);
		}
		else
		{
			uint16_t payloadPacketNumber = 0;

			/* Add the SAT ID and frame type data to the packet first. */
			AddSatelliteIDBytes(&bitStream, &sendBuffer[sendBufferIndex][0], PAYLOAD_DATA_FRAME);

			/* Fetch the payload data and place into the buffer. */
			if(GetPayloadData(&sendBuffer[sendBufferIndex][START_OF_PAYLOAD_PACKET_DATA_SECTION], SIZE_OF_PAYLOAD_DATA_PACKET_SECTION, &payloadPacketNumber) == NO_ERROR_PAYLOAD_DATA)
			{
				/* Add our packet number to the start of the packet. */
				AddToBitStream(&bitStream, &sendBuffer[sendBufferIndex][0], 16, payloadPacketNumber);	
			}
			else
			{
				/* Invalid data so we mark packet as being invalid and clear the data sent down. */
				AddToBitStream(&bitStream, &sendBuffer[sendBufferIndex][0], 16, 0xFFFF);
				memset(&sendBuffer[sendBufferIndex][START_OF_PAYLOAD_PACKET_DATA_SECTION], 0, SIZE_OF_PAYLOAD_DATA_PACKET_SECTION);
			}
		}

		/* We are in 4k8 mode so we need to get data and FEC it ready for TX next cycle */
		outputFEC = EncodeFEC(&sendBuffer[sendBufferIndex][0], &count);

		/* Copy our FECed output into the correct place to be sent. */
		memcpy(&sendBuffer[sendBufferIndex][0], outputFEC, count);

		/* Flip to the other buffer for next cycle. */
		sendBufferIndex = (sendBufferIndex >= 1) ? 0 : 1;
	}
	else
	{
		/* An error has occurred. */
	}
}

/* Interface function to tell the uplink to change mode when ready. */
void ChangeDownlinkMode(DATA_MODE z_mode)
{
	/* Ensure our enumeration is valid. */
	if(z_mode == MODE_RX_ONLY || z_mode == MODE_1K2 || z_mode == MODE_4K8 || z_mode == MODE_4K8_PAYLOAD)
	{
		dataModeNew = z_mode;
	}
	/* Change mode now */
	ActionChangeDownlinkMode();
}

/* Local function to change the current operating mode
   based upon commands received through ChangeDownlinkMode() */
static void ActionChangeDownlinkMode(void)
{
	if(dataMode != dataModeNew)
	{
		BITSTREAM_CONTROL bitStream;	// Create a bit stream

		/* Initialise the bit stream. */
		memset(&bitStream, 0 , sizeof(bitStream));

		/* Update the data mode. */
		dataMode = dataModeNew;

		/* Reset pieces for after the change. */
		frameNumber = 1;
		rttPacketNumber = 0;
		packetSequenceNumber = 0;
			
		/* If we have changed into 4k8 mode then we need to prep the first packet to send once the current packet is finished. */
		if(dataMode == MODE_4K8 || dataMode == MODE_4K8_PAYLOAD)
		{
			uint16_t count = 0;
			uint8_t * outputFEC = 0;

			/* Check we havent had an SEUs */
			sendBufferIndex &= 0x01;

			/* Clear the output buffer section we arent currently transmitting. */
			memset(&sendBuffer[sendBufferIndex][0], 0, (sizeof(sendBuffer) / NUMBER_OF_PREPERATION_BUFFERS));

			/* Add the SAT ID and frame type data to the packet first. */
			AddSatelliteIDBytes(&bitStream, &sendBuffer[sendBufferIndex][0], 1);

			/* Fetch all telemetry data for RTT format 1 out of 2. */
			FetchTelemetryData(&bitStream, 0, &sendBuffer[sendBufferIndex][0]);

			/* Get the specific data required by frame 1 */
			FetchFrameSpecificData(&bitStream, 1, &sendBuffer[sendBufferIndex][0]);			
				
			/* Perform the FEC of the data to be sent. */
			outputFEC = EncodeFEC(&sendBuffer[sendBufferIndex][0], &count);

			/* Copy into the buffer to be sent next time. */
			memcpy(&sendBuffer[sendBufferIndex][0], outputFEC, count);

			/* Flip the send buffer */
			sendBufferIndex = (sendBufferIndex >= 1) ? 0 : 1;
			/* Record that we have sent the first RTT packet. */
			rttPacketNumber = (rttPacketNumber >= 1) ? 0 : 1;
			/* increment our frame number */
			frameNumber++;
		}
	}
}

/* Add the standard set of sat ID and frame type bytes to the bit stream packet to be sent. */
static void AddSatelliteIDBytes(BITSTREAM_CONTROL * z_bitStream, uint8_t * z_buffer, uint8_t z_frameNumber)
{
	/* Add the packet type, frame number and sat ID. */
	AddToBitStream(z_bitStream, z_buffer, 2, 0x3);				// Use extended ID field.
	AddToBitStream(z_bitStream, z_buffer, 1, 0x0);				// Dont enable debug flag.
	AddToBitStream(z_bitStream, z_buffer, 5, z_frameNumber);	// Include the frame number.
	AddToBitStream(z_bitStream, z_buffer, 6, 1);				// Mark as being ESEO
	AddToBitStream(z_bitStream, z_buffer, 2, 0);				// Also part of frame type field but always zero. 
}

/* This function will collect data to be transmitted based upon the frame number passed in - this is in accordance
   with the AMSAT frame definition specification. */
static uint32_t FetchFrameSpecificData(BITSTREAM_CONTROL * z_bitStream, uint8_t z_frameNumber, uint8_t * z_buffer)
{
	
	if(z_bitStream != 0 && z_buffer != 0)
	{
		if(z_frameNumber == FITTER_MESSAGE_1 || z_frameNumber == FITTER_MESSAGE_2 || z_frameNumber == FITTER_MESSAGE_3 || z_frameNumber == FITTER_MESSAGE_4) 
		{
			/* Explicitly configure bit stream to add fitter data to the correct place. */
			z_bitStream->bitInput = 0;
			z_bitStream->byteInput = START_OF_PAYLOAD_DATA_SECTION;
			FetchFitterMessageFrameData(z_bitStream, z_frameNumber, z_buffer);
		}
		else if(z_frameNumber <= NUMBER_OF_FRAME_TYPES)
		{
			/* Explicitly configure bit stream to add fitter data to the correct place. */
			z_bitStream->bitInput = 0;
			z_bitStream->byteInput = START_OF_PAYLOAD_DATA_SECTION;
			FetchPayloadDataFrameData(z_bitStream, z_frameNumber, z_buffer);
		}
		else
		{
			/* Somethings gone wrong! */
		}
	}
}



static void FetchPayloadDataFrameData(BITSTREAM_CONTROL * z_bitStream, uint8_t z_frameNumber, uint8_t * z_buffer)
{
	uint16_t test;
	
	if(z_bitStream != 0 && z_buffer != 0)
	{
		uint8_t i = 0;

		/* Need to subtract for each of the fitter messages */
		if(z_frameNumber > FITTER_MESSAGE_1 && z_frameNumber < FITTER_MESSAGE_2)
		{
			z_frameNumber -= 1;
		}
		else if(z_frameNumber > FITTER_MESSAGE_2 && z_frameNumber < FITTER_MESSAGE_3)
		{
			z_frameNumber -= 2;
		}
		else if(z_frameNumber > FITTER_MESSAGE_3 && z_frameNumber < FITTER_MESSAGE_4)
		{
			z_frameNumber -= 3;
		}
		else if(z_frameNumber > FITTER_MESSAGE_4 && z_frameNumber <= NUMBER_OF_FRAME_TYPES)
		{
			z_frameNumber -= 4;
		}
		else
		{
			/* Something is wrong. */
		}

		/* Fetch the required WOD data */
		GetWODData(z_frameNumber, fitterBuffer);
		
		/* Add each byte to the bit stream. */
		for(i = 0; i < WOD_SIZE; i++)
		{
			AddToBitStream(z_bitStream, z_buffer, 8, (uint32_t)fitterBuffer[i]);
		}		
	}
}

/* Fetch the fitter message data to be added to the bit stream. */
static void FetchFitterMessageFrameData(BITSTREAM_CONTROL * z_bitStream, uint8_t z_frameNumber, uint8_t * z_buffer)
{
	uint8_t i = 0;
	if(z_bitStream != 0 && z_buffer != 0)
	{
		/* Get the appropriate fitter message. */
		if(z_frameNumber == FITTER_MESSAGE_1)
		{
			GetFitterMessage(FITTER_SLOT_0, fitterBuffer, FITTER_MESSAGE_SIZE);
		}
		else if(z_frameNumber == FITTER_MESSAGE_2)
		{
			GetFitterMessage(FITTER_SLOT_1, fitterBuffer, FITTER_MESSAGE_SIZE);
		}
		else if(z_frameNumber == FITTER_MESSAGE_3)
		{
			GetFitterMessage(FITTER_SLOT_2, fitterBuffer, FITTER_MESSAGE_SIZE);
		}
		else if(z_frameNumber == FITTER_MESSAGE_4)
		{
			GetFitterMessage(FITTER_SLOT_3, fitterBuffer, FITTER_MESSAGE_SIZE);
		}

		/* Add each byte to the bit stream. */
		for(i = 0; i < FITTER_MESSAGE_SIZE; i++)
		{
			AddToBitStream(z_bitStream, z_buffer, 8, (uint32_t)fitterBuffer[i]);
		}
	}
}

/* Fetch telemetry data - the sequence number is the packet sequence number
   that is required to be sent - see AMSAT documentation. */
static uint32_t FetchTelemetryData(BITSTREAM_CONTROL * z_bitStream, uint8_t z_rttNumber, uint8_t * z_buffer)
{
	uint32_t data = 0;
	
	if(z_bitStream != 0 && z_buffer != 0)
	{
		/* Add the common telemetry section */
		AddCommonTelemetrySection(z_bitStream, z_buffer);

		/* Flip / flop between two packets to be sent. */
		if(z_rttNumber == 0)
		{
			AddRTTTelemetrySection1(z_bitStream, z_buffer);
		}
		else if(z_rttNumber == 1)
		{
			AddRTTTelemetrySection2(z_bitStream, z_buffer);
		}
		else
		{
			/* Shouldn't ever happen but try to clean up */
			z_rttNumber = 0;
		}
	}
	/* Return the number of bits encoded into the buffer. */
	return ((z_bitStream->byteInput * 8) + z_bitStream->bitInput);
}

/* All RTT packets have a common section (the majority) - this function will fetch all of the data required. */
static void AddCommonTelemetrySection(BITSTREAM_CONTROL * z_control, uint8_t * z_buffer)
{
	uint32_t data = 0;
	if(z_control != 0 && z_buffer != 0)
	{
		GetCANObjectDataRaw(AMS_EPS_DCDC_V, &data);
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_EPS_DCDC_I, &data);
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_EPS_DCDC_T, &data);							  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_EPS_ENC_T, &data);							  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_EPS_CCT_T, &data);							  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_EPS_3V3_V, &data);							  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_EPS_3V3_I, &data);							  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_EPS_6V_V, &data);							  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_EPS_6V_I, &data);							  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_EPS_9V_V, &data);							  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_EPS_9V_I, &data);							  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_VHF_FP, &data);								  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_VHF_RP, &data);								  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_VHF_FM_PA_T, &data);						  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_VHF_BPSK_PA_T, &data);						  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_VHF_BPSK_PA_I, &data);						  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_VHF_BPSK_I, &data);							  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_L_RSSI_TRANS, &data);						  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_L_RSSI_COMMAND, &data);						  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_L_DOPPLER_COMMAND, &data);					  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_L_T, &data);								  
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_OBC_SEQNUM, &data);	
		AddToBitStream(z_control, z_buffer, 24, data);
		GetCANObjectDataRaw(AMS_OBC_LC, &data);			
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(AMS_OBC_RFMODE, &data);							  
		AddToBitStream(z_control, z_buffer, 3, data);
		GetCANObjectDataRaw(AMS_OBC_DATAMODE, &data);						  
		AddToBitStream(z_control, z_buffer, 2, data);
		GetCANObjectDataRaw(AMS_OBC_PL_TRANS_STAT, &data);
		AddToBitStream(z_control, z_buffer, 1, data);
		GetCANObjectDataRaw(AMS_OBC_IN_ECLIPSE, &data);
		AddToBitStream(z_control, z_buffer, 1, data);
		GetCANObjectDataRaw(AMS_OBC_ECLIPSE_MODE, &data);
		AddToBitStream(z_control, z_buffer, 1, data);	
		GetCANObjectDataRaw(AMS_OBC_CTCSS_MODE, &data);	
		AddToBitStream(z_control, z_buffer, 1, data);
		GetCANObjectDataRaw(AMS_OBC_AUTO_SAFE_MODE, &data);													  
		AddToBitStream(z_control, z_buffer, 1, data);
		GetCANObjectDataRaw(AMS_OBC_SAFE_MODE_ACTIVE, &data);									
		AddToBitStream(z_control, z_buffer, 1, data);	
	}
}

/* This function will fetch the non-common chunk of RTT packet #1. */
static void AddRTTTelemetrySection1(BITSTREAM_CONTROL * z_control, uint8_t * z_buffer)
{
	uint32_t data;
	if(z_control != 0 && z_buffer != 0)
	{
		GetCANObjectDataRaw(PPM_VOLTAGE_SP1, &data);	
		AddToBitStream(z_control, z_buffer, 16, data);
		GetCANObjectDataRaw(PPM_VOLTAGE_SP2, &data);	
		AddToBitStream(z_control, z_buffer, 16, data);
		GetCANObjectDataRaw(PPM_VOLTAGE_SP3, &data);	
		AddToBitStream(z_control, z_buffer, 16, data);
		GetCANObjectDataRaw(OBD_MODE, &data);	
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(OBD_EQUIPMENT_STATUS, &data);		   
		AddToBitStream(z_control, z_buffer, 32, data);
		GetCANObjectDataRaw(OBD_WD_RESET_COUNT, &data);			   
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(ACS_OMEGA_P, &data);				   
		AddToBitStream(z_control, z_buffer, 32, data);
		GetCANObjectDataRaw(ACS_OMEGA_Q, &data);				   
		AddToBitStream(z_control, z_buffer, 32, data);
		GetCANObjectDataRaw(ACS_OMEGA_R, &data);				   
		AddToBitStream(z_control, z_buffer, 32, data);
	}
}

/* This function will fetch the non-common chunk of RTT packet #2. */
static void AddRTTTelemetrySection2(BITSTREAM_CONTROL * z_control, uint8_t * z_buffer)
{
	uint32_t data;
	if(z_control != 0 && z_buffer != 0)
	{
		GetCANObjectDataRaw(ACS_OMEGA_X, &data);
		AddToBitStream(z_control, z_buffer, 32, data);
		GetCANObjectDataRaw(ACS_OMEGA_Y, &data);
		AddToBitStream(z_control, z_buffer, 32, data);
		GetCANObjectDataRaw(ACS_OMEGA_Z, &data);
		AddToBitStream(z_control, z_buffer, 32, data);
		GetCANObjectDataRaw(STX_TEMP_4, &data);
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(PMM_AMSAT_CURRENT, &data);
		AddToBitStream(z_control, z_buffer, 16, data);
		GetCANObjectDataRaw(MWM_VOLTAGE, &data);
		AddToBitStream(z_control, z_buffer, 8, data);
		GetCANObjectDataRaw(MWM_CURRENT, &data);
		GetCANObjectDataRaw(MWM_OMEGAMESURED, &data);
		AddToBitStream(z_control, z_buffer, 16, data);
		GetCANObjectDataRaw(MPS_HPT01, &data);
		AddToBitStream(z_control, z_buffer, 16, data);
		GetCANObjectDataRaw(PMM_TEMP_SP1_SENS_1, &data);
		AddToBitStream(z_control, z_buffer, 16, data);
		GetCANObjectDataRaw(PMM_TEMP_BP1_SENS_1, &data);
		AddToBitStream(z_control, z_buffer, 16, data);
	}
}

static void UpdatePacketCounters(void)
{
	/* Update all counter variables */
	if(packetCounterUpdate != 0)
	{
		packetCounterUpdate = 0;
		if(commandQueueHandle != 0)
		{
			uint8_t buffer[CAN_QUEUE_ITEM_SIZE];
			/* Record that this data is from the uplink. */
			buffer[PACKET_SOURCE_INDEX] = ON_TARGET_DATA;
			/* Add the packet length to the queue packet. */
			buffer[PACKET_SIZE_INDEX] = 5;			
			buffer[OBJECT_DICTIONARY_ID_INDEX] = AMS_OBC_P_DOWN;
			memcpy(&buffer[PACKET_DATA_INDEX], &packetsDownlinked, sizeof(packetsDownlinked));
			xQueueSend(commandQueueHandle, buffer, 0);
			buffer[OBJECT_DICTIONARY_ID_INDEX] = AMS_OBC_FRAME;
			memcpy(&buffer[PACKET_DATA_INDEX], &frameNumber, sizeof(frameNumber));
			xQueueSend(commandQueueHandle, buffer, 0);
		}
	}
}

static void packetCounterUpdateTimerCallback(xTimerHandle xTimer)
{
	packetCounterUpdate = 1;
}
