#ifndef CANODINTERFACE_H_
#define CANODINTERFACE_H_

#include "FreeRTOS.h"
#include "queue.h"

/* 
   These enumeration should be used for writing to a specific 
   location in the CAN object dictionary
*/
typedef enum _CAN_OD_ENTRIES
{
	NODE_ID						= 0x00,	
	DAYS_PAST_J2000				= 0x01,
	MILLISECONDS_OF_DAY			= 0x02,
	AMS_OBC_THREAD_HB			= 0x09,
	AMS_OBC_CMD_WDT_TIME_REM	= 0x0A,
	AMS_OBC_CAN_PL_PCKTS_REM    = 0x0B,
	AMS_OBC_ECLIPSE_PANELS_DEF	= 0x0C,
	AMS_OBC_IN_ECLIPSE			= 0x0D,
	AMS_OBC_PL_TRANS_STAT		= 0x0E,
	AMS_OBC_SAFE_MODE_ACTIVE	= 0x0F,
	AMS_OBC_RFMODE              = 0x10,
	AMS_OBC_RFMODE_DEF			= 0x11,
	AMS_OBC_DATAMODE			= 0x12,
	AMS_OBC_DATAMODE_DEF		= 0x13,
	AMS_OBC_PAYLOAD_FTP			= 0x14,
	AMS_OBC_HM_INDEX			= 0x15,
	AMS_OBC_HM_REGISTER			= 0x16,
	AMS_OBC_FM_SLOT				= 0x17,
	AMS_OBC_FM_INDEX			= 0x18,
	AMS_OBC_FM_REGISTER			= 0x19,
	AMS_OBC_ECLIPSE_THRESH		= 0x1A,
	AMS_OBC_ECLIPSE_PANELS		= 0x1B,
	AMS_OBC_ECLIPSE_MODE		= 0x1C,
	AMS_OBC_ECLIPSE_MODE_DEF	= 0x1D,
	AMS_OBC_CTCSS_MODE			= 0x1E,
	AMS_OBC_AUTO_SAFE_MODE		= 0x1F,
	AMS_OBC_DEBUG_INDEX			= 0x20,
	AMS_OBC_DEBUG_REGISTER		= 0x21,
	AMS_OBC_FRAME				= 0x22,
	AMS_OBC_SEQNUM				= 0x23,
	AMS_OBC_LC					= 0x24,
	AMS_OBC_HB_WDT				= 0x25,
	AMS_OBC_LC_WDT				= 0x26,
	AMS_OBC_TEMP_PROT			= 0x27,
	AMS_OBC_P_UP				= 0x28,
	AMS_OBC_P_UP_DROPPED		= 0x29,
	AMS_OBC_P_DOWN				= 0x2A,
	AMS_OBC_CAN_STATUS			= 0x2B,
	AMS_OBC_CAN_PL              = 0x2C,
	AMS_OBC_CAN_PL_STATUS		= 0x2D,
	AMS_OBC_MEM_STAT_1			= 0x2E,
	AMS_OBC_MEM_STAT_2			= 0x2F,
	AMS_OBC_FLASH_RATE			= 0x30,
	AMS_EPS_DCDC_V				= 0x31,
	AMS_EPS_DCDC_I				= 0x32,
	AMS_EPS_DCDC_T				= 0x33,
	AMS_EPS_ENC_T				= 0x34,
	AMS_EPS_CCT_T				= 0x35,
	AMS_EPS_3V3_V				= 0x36,
	AMS_EPS_3V3_I				= 0x37,
	AMS_EPS_6V_V				= 0x38,
	AMS_EPS_6V_I				= 0x39,
	AMS_EPS_9V_V				= 0x3A,
	AMS_EPS_9V_I				= 0x3B,
	AMS_VHF_FP					= 0x3C,
	AMS_VHF_RP					= 0x3D,
	AMS_VHF_FM_PA_T				= 0x3E,
	AMS_VHF_BPSK_PA_T			= 0x3F,
	AMS_VHF_BPSK_PA_I			= 0x40,
	AMS_VHF_BPSK_I				= 0x41,
	AMS_VHF_FM_PA_I				= 0x42,
	AMS_L_RSSI_TRANS			= 0x43,
	AMS_L_RSSI_COMMAND			= 0x44,
	AMS_L_DOPPLER_COMMAND		= 0x45,
	AMS_L_T						= 0x46,
	OBD_MODE					= 0x47,
	OBD_EQUIPMENT_STATUS		= 0x48,
	OBD_WD_RESET_COUNT			= 0x49,
	STX_TEMP_4					= 0x4A,
	ACS_OMEGA_P					= 0x4B,
	ACS_OMEGA_Q					= 0x4C,
	ACS_OMEGA_R					= 0x4D,
	ACS_OMEGA_X					= 0x4E,
	ACS_OMEGA_Y					= 0x4F,
	ACS_OMEGA_Z					= 0x50,
	ACS_OMEGA_VX				= 0x51,
	ACS_OMEGA_VY				= 0x52,
	ACS_OMEGA_VZ				= 0x53,
	MWM_VOLTAGE					= 0x54,
	MWM_CURRENT					= 0x55,
	MWM_OMEGAMESURED			= 0x56,
	MPS_HPT01					= 0x57,
	PMM_TEMP_SP1_SENS_1			= 0x58,
	PMM_TEMP_SP2_SENS_1			= 0x59,
	PMM_TEMP_SP3_SENS_1			= 0x5A,
	PMM_CURRENT_BP1				= 0x5B,
	PMM_CURRENT_BP4				= 0x5C,
	PMM_TEMP_BP1_SENS_1			= 0x5D,
	PMM_AMSAT_CURRENT			= 0x5E,
	PMM_VOLTAGE_MB				= 0x5F,
	PPM_VOLTAGE_SP1				= 0x60,
	PPM_VOLTAGE_SP2				= 0x61,
	PPM_VOLTAGE_SP3				= 0x62
}CAN_OD_ENTRIES;				  

QueueHandle_t GetCommandQueueHandle(void);

#define PACKET_SOURCE_INDEX				0		// Location in the packet buffer for the field indicating the origin of the packet.
#define PACKET_SIZE_INDEX				1
#define OBJECT_DICTIONARY_ID_INDEX		2
#define PACKET_DATA_INDEX				3

#define ON_TARGET_DATA				0		// This indicates the packet originates from on this target - the data is big endian.
#define OFF_TARGET_DATA				1		// This indicates the packet originates from on this target - the data is little endian.

#define CAN_QUEUE_ITEM_SIZE 29				// 25 for message, 1 for location, 1 for length, 2 for CRC (we ignore the CRC but need buffer space.)

#define HOLEMAP_MAX_SIZE_BYTES	33
#define PAYLOAD_DATA_PACKET_SIZE 252

void GetHolemap(uint8_t * z_buffer, uint16_t z_size);
void GetFitterMessage(uint8_t z_slot, uint8_t * z_buffer, uint16_t z_size);
void ClearHoleMap(void);

#endif /* CANODINTERFACE_H_ */