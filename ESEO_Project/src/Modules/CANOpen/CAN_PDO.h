#ifndef CAN_PDO_H_
#define CAN_PDO_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "CANOpen.h"

CAN_OPEN_ERROR	ProcessDataObjectGET(const CAN_PACKET * z_incomingPacket, CAN_PACKET * z_outgoingPacket);
CAN_OPEN_ERROR	ProcessDataObjectSET(const CAN_PACKET * z_incomingPacket, CAN_PACKET * z_outgoingPacket);
void ProcessDataObjectSETT(const CAN_PACKET * z_incomingPacket);

CAN_OPEN_ERROR ReceivedPayloadDataTransferProtocolRequest(uint8_t z_id, uint32_t z_data, uint8_t z_changed);
CAN_OPEN_ERROR ReceivedPayloadDataTransferProtocolAcknowledge(const CAN_PACKET * z_incomingPacket);
CAN_OPEN_ERROR CANPDOPayloadTransferProtocolUpdate(void);
uint8_t PayloadDataTransferIsInProgess(void);
CAN_OPEN_ERROR InitialiseProcessDataObjects(QueueHandle_t z_payloadStorageQueue);
uint32_t GetPayloadDataPacketsTransferred(void);

#endif