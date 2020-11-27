/* This file is the main uplink interface.
	There are two methods that are used for placing and removing data from this
	demodulator and decoder - the input stream comes as readings from the ADC 
	and the output comes in the form of packets that are ready to be handled.
*/

#ifndef UPLINK_H_
#define UPLINK_H_

#include <stdint.h>	
#include "Demodulator.h"

#define CRC_PACKET_LENGTH 2
#define PACKET_MAXIMUM_LENGTH 25 + CRC_PACKET_LENGTH	// Maximum packet data length 25 for data, 2 for the CRC

typedef struct _UPLINK_PACKET
{
	uint8_t packetLength;
	uint8_t data[PACKET_MAXIMUM_LENGTH];
}UPLINK_PACKET;

void UplinkInit(void);								// Perform required initialisations.
void RegisterUplinkInput(uint32_t((*)(int32_t *)));	// Register the GetSampleFromBuffer function with the uplink block.
uint8_t UplinkExecute(void);						// Main uplink demodulation and decoding execution function.
void UplinkThread(void);							// Uplink thread.
uint8_t GetUplinkPacketCount(void);					// Returns the number of packets waiting to be read.
void PutUplinkPacket(UPLINK_PACKET z_packet);		// Place a single uplink packet into the uplink packet buffer. (Not meant to be used externally - didnt want to faff with function pointers).
void PacketRejected(void);							// Record that a packet has been rejected.
#endif /* UPLINK_H_ */