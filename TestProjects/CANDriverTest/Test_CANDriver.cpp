#include "gtest/gtest.h"

extern "C"
{
	#include "CANDriver.h"
	#include "fff.h"
	#include "scif_uc3c.h"
	#include "Fake_scif_uc3c.h"
	#include "canif.h"
	#include "Fake_canif.h"
	#include "Fake_gpio.h"
	#include "intc.h"
	#include "Fake_intc.h"
	#include "uc3c0512c.h"
}

extern volatile can_msg_t *p_messageObjectBufferChannel0;
extern uint32_t * p_messageObjectAllocationTable;
extern void((*p_CANTransmitOK_ISR)(void));
extern void((*p_CANReceiveOK_ISR)(void));
extern void((*p_CANBusOff_ISR)(void));
extern void((*p_CANError_ISR)(void));
extern void((*p_CANWakeUp_ISR)(void));
DEFINE_FFF_GLOBALS;

#define FILL_ALL_TX_MOBS for(uint32_t i = 0; i < 4; i++) TransmitCAN(testPacket); 
#define FREE_MOB(mob) 	CANIF_mob_get_mob_txok_fake.return_val = mob; p_CANTransmitOK_ISR();

uint32_t CANIF_channel_enable_status_fakeHistory[2];

class CANDriver : public testing::Test
{
public:

	void SetUp()
	{
		RESET_FAKE(scif_gc_setup);
		RESET_FAKE(scif_gc_enable);
		RESET_FAKE(gpio_enable_module);
		RESET_FAKE(CANIF_disable);
		RESET_FAKE(CANIF_set_reset);
		RESET_FAKE(CANIF_channel_enable_status);
		RESET_FAKE(CANIF_clr_reset);
		RESET_FAKE(CANIF_set_ram_add);
		RESET_FAKE(CANIF_bit_timing);
		RESET_FAKE(CANIF_set_channel_mode);
		RESET_FAKE(CANIF_clr_overrun_mode);
		RESET_FAKE(canif_clear_all_mob);
		RESET_FAKE(CANIF_enable);
		RESET_FAKE(INTC_register_interrupt);
		RESET_FAKE(CANIF_enable_interrupt);
		RESET_FAKE(CANIF_set_std_id);
		RESET_FAKE(CANIF_mob_clr_dlc);
		RESET_FAKE(CANIF_mob_set_dlc);
		RESET_FAKE(CANIF_set_data);
		RESET_FAKE(CANIF_config_tx);
		RESET_FAKE(CANIF_mob_enable);
		RESET_FAKE(CANIF_mob_enable_interrupt);
		RESET_FAKE(CANIF_set_std_idmask);
		RESET_FAKE(CANIF_config_rx);
		RESET_FAKE(CANIF_mob_get_mob_rxok);
		RESET_FAKE(CANIF_mob_clear_rxok_status);
		RESET_FAKE(CANIF_mob_clear_status);
		RESET_FAKE(CANIF_disable_interrupt);
		RESET_FAKE(CANIF_disable);
		RESET_FAKE(CANIF_mob_disable);
		RESET_FAKE(CANIF_mob_disable);
		RESET_FAKE(CANIF_set_sjw);
		RESET_FAKE(CANIF_set_prs);
		RESET_FAKE(CANIF_set_pres);
		RESET_FAKE(CANIF_set_phs2);
		RESET_FAKE(CANIF_set_phs1);

		FFF_RESET_HISTORY();
		*p_messageObjectAllocationTable = 0;

		CANIF_channel_enable_status_fakeHistory[0] = 0;
		CANIF_channel_enable_status_fakeHistory[1] = 1;
		SET_RETURN_SEQ(CANIF_channel_enable_status, CANIF_channel_enable_status_fakeHistory, 2);
	}

	virtual void TearDown() 
	{

	}
};

/***********************************************************/
/* Test Intialisation Routines.							   */
/***********************************************************/
TEST_F(CANDriver, TestThatAllDriverInitialisationMethodsAreCalledCorrectly)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(1, scif_gc_setup_fake.call_count);
	EXPECT_EQ(1, scif_gc_enable_fake.call_count);
	EXPECT_EQ(1, gpio_enable_module_fake.call_count);
	EXPECT_EQ(1, CANIF_disable_fake.call_count);
	EXPECT_EQ(1, CANIF_set_reset_fake.call_count);
	EXPECT_EQ(2, CANIF_channel_enable_status_fake.call_count);
	EXPECT_EQ(1, CANIF_clr_reset_fake.call_count);
	EXPECT_EQ(1, CANIF_set_ram_add_fake.call_count);
	EXPECT_EQ(1, CANIF_set_sjw_fake.call_count);
	EXPECT_EQ(1, CANIF_set_prs_fake.call_count);
	EXPECT_EQ(1, CANIF_set_pres_fake.call_count);
	EXPECT_EQ(1, CANIF_set_phs2_fake.call_count);
	EXPECT_EQ(1, CANIF_set_phs1_fake.call_count);
	EXPECT_EQ(1, CANIF_set_channel_mode_fake.call_count);
	EXPECT_EQ(1, CANIF_clr_overrun_mode_fake.call_count);
	EXPECT_EQ(1, canif_clear_all_mob_fake.call_count);
	EXPECT_EQ(1, CANIF_enable_fake.call_count);
	EXPECT_EQ(10, INTC_register_interrupt_fake.call_count);
	EXPECT_EQ(1, CANIF_enable_interrupt_fake.call_count);
}

TEST_F(CANDriver, TestThatAllRXInitialisationMethodsAreCalledCorrectly)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(11, CANIF_set_std_id_fake.call_count);
	EXPECT_EQ(11, CANIF_set_std_idmask_fake.call_count);
	EXPECT_EQ(11, CANIF_config_rx_fake.call_count);
	EXPECT_EQ(11, CANIF_mob_enable_fake.call_count);
	EXPECT_EQ(11, CANIF_mob_enable_interrupt_fake.call_count);
}

TEST_F(CANDriver, TestValuePassedInto_scif_gc_setup)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(1, scif_gc_setup_fake.arg0_val);
	EXPECT_EQ(3, scif_gc_setup_fake.arg1_val);
	EXPECT_EQ(0, scif_gc_setup_fake.arg2_val);
	EXPECT_EQ(0, scif_gc_setup_fake.arg3_val);
}

TEST_F(CANDriver, TestValuePassedInto_scif_gc_enable)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(1, scif_gc_setup_fake.arg0_val);
}

TEST_F(CANDriver, TestValuePassedInto_gpio_enable_module)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	gpio_map_t_intermideary * gpio_enable_module_argment_pointer = (gpio_map_t_intermideary *)gpio_enable_module_fake.arg0_val;
	EXPECT_EQ(gpio_enable_module_argment_pointer[0].function, AVR32_CANIF_RXLINE_0_0_FUNCTION);
	EXPECT_EQ(gpio_enable_module_argment_pointer[0].pin, AVR32_CANIF_RXLINE_0_0_PIN);
	EXPECT_EQ(gpio_enable_module_argment_pointer[1].function, AVR32_CANIF_TXLINE_0_0_FUNCTION);
	EXPECT_EQ(gpio_enable_module_argment_pointer[1].pin, AVR32_CANIF_TXLINE_0_0_PIN);
	EXPECT_EQ(gpio_enable_module_argment_pointer[2].function, AVR32_CANIF_RXLINE_1_2_FUNCTION);
	EXPECT_EQ(gpio_enable_module_argment_pointer[2].pin, AVR32_CANIF_RXLINE_1_2_PIN);
	EXPECT_EQ(gpio_enable_module_argment_pointer[3].function, AVR32_CANIF_TXLINE_1_2_FUNCTION);
	EXPECT_EQ(gpio_enable_module_argment_pointer[3].pin, AVR32_CANIF_TXLINE_1_2_PIN);
}

TEST_F(CANDriver, TestValuesUsedToConfigureBaudRate)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(0, CANIF_set_sjw_fake.arg0_val);
	EXPECT_EQ(0, CANIF_set_prs_fake.arg0_val);
	EXPECT_EQ(0, CANIF_set_pres_fake.arg0_val);
	EXPECT_EQ(0, CANIF_set_phs2_fake.arg0_val);
	EXPECT_EQ(0, CANIF_set_phs1_fake.arg0_val);
	EXPECT_EQ(1, CANIF_set_sjw_fake.arg1_val);
	EXPECT_EQ(5, CANIF_set_prs_fake.arg1_val);
	EXPECT_EQ(3, CANIF_set_pres_fake.arg1_val);
	EXPECT_EQ(2, CANIF_set_phs2_fake.arg1_val);
	EXPECT_EQ(5, CANIF_set_phs1_fake.arg1_val);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_disable)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(0, CANIF_disable_fake.arg0_val);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_set_reset)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(0, CANIF_set_reset_fake.arg0_val);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_channel_enable_status)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(0, CANIF_channel_enable_status_fake.arg0_val);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_clr_reset)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(0, CANIF_clr_reset_fake.arg0_val);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_set_ram_add)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(0, CANIF_set_ram_add_fake.arg0_val);
	EXPECT_EQ((unsigned long)p_messageObjectBufferChannel0, CANIF_set_ram_add_fake.arg1_val);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_bit_timing)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(0, CANIF_bit_timing_fake.arg0_val);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_set_channel_mode)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(0, CANIF_set_channel_mode_fake.arg0_val);
	EXPECT_EQ(0, CANIF_set_channel_mode_fake.arg1_val);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_clr_overrun_mode)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(0, CANIF_clr_overrun_mode_fake.arg0_val);
}

TEST_F(CANDriver, TestValuePassedInto_canif_clear_all_mob)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(0, canif_clear_all_mob_fake.arg0_val);
	EXPECT_EQ(16, canif_clear_all_mob_fake.arg1_val);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_enable)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(0, CANIF_enable_fake.arg0_val);
}

TEST_F(CANDriver, TestValuePassedInto_INTC_register_interrupt)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(p_CANTransmitOK_ISR, INTC_register_interrupt_fake.arg0_history[0]);
	EXPECT_EQ(p_CANReceiveOK_ISR, INTC_register_interrupt_fake.arg0_history[1]);
	EXPECT_EQ(p_CANBusOff_ISR, INTC_register_interrupt_fake.arg0_history[2]);
	EXPECT_EQ(p_CANError_ISR, INTC_register_interrupt_fake.arg0_history[3]);
	EXPECT_EQ(p_CANWakeUp_ISR, INTC_register_interrupt_fake.arg0_history[4]);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_enable_interrupt)
{
	uint16_t cobID[11];
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(0, CANIF_enable_interrupt_fake.arg0_val);
}

TEST_F(CANDriver, TestNullPointerIsRejected)
{
	uint16_t cobID = 1;
	
	EXPECT_NE(NO_ERROR_CAN, InitialiseCANDriver((uint16_t*)0, 1));
}

TEST_F(CANDriver, TestValidNumberOfChannnelsIsAccepted)
{
	uint16_t cobID[11];
	
	EXPECT_EQ(NO_ERROR_CAN, InitialiseCANDriver(cobID, 11));
}

TEST_F(CANDriver, TestInvalidNumberOfChannnelsIsRejected)
{
	uint16_t cobID[13];
	
	EXPECT_NE(NO_ERROR_CAN, InitialiseCANDriver(cobID, 13));
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_set_std_id)
{
	uint16_t cobID[11] = {1,2,3,4,5,6,7,8,9,10,11};
	InitialiseCANDriver(cobID, 11);
	EXPECT_EQ(1, CANIF_set_std_id_fake.arg2_history[0]);
	EXPECT_EQ(2, CANIF_set_std_id_fake.arg2_history[1]);
	EXPECT_EQ(3, CANIF_set_std_id_fake.arg2_history[2]);
	EXPECT_EQ(4, CANIF_set_std_id_fake.arg2_history[3]);
	EXPECT_EQ(5, CANIF_set_std_id_fake.arg2_history[4]);
	EXPECT_EQ(6, CANIF_set_std_id_fake.arg2_history[5]);
	EXPECT_EQ(7, CANIF_set_std_id_fake.arg2_history[6]);
	EXPECT_EQ(8, CANIF_set_std_id_fake.arg2_history[7]);
	EXPECT_EQ(9, CANIF_set_std_id_fake.arg2_history[8]);
	EXPECT_EQ(10, CANIF_set_std_id_fake.arg2_history[9]);
	EXPECT_EQ(11, CANIF_set_std_id_fake.arg2_history[10]);
}
/***********************************************************/
/* Test Transmit Routine.							       */
/***********************************************************/
TEST_F(CANDriver, TestCorrectDataSizeReturnsOK)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;
	EXPECT_EQ(NO_ERROR_CAN, TransmitCAN(testPacket));
}

TEST_F(CANDriver, TestIncorrectDataSizeReturnsError)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 9;
	EXPECT_EQ(ERROR_DATA_LENGTH_TOO_LONG, TransmitCAN(testPacket));
}

TEST_F(CANDriver, TestTransmitDoesntTransmitWhenDataLengthWrong)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 9;
	TransmitCAN(testPacket);
	EXPECT_EQ(0, CANIF_set_std_id_fake.call_count);
	EXPECT_EQ(0, CANIF_mob_clr_dlc_fake.call_count);
	EXPECT_EQ(0, CANIF_mob_set_dlc_fake.call_count);
	EXPECT_EQ(0, CANIF_set_data_fake.call_count);
	EXPECT_EQ(0, CANIF_config_tx_fake.call_count);
	EXPECT_EQ(0, CANIF_mob_enable_fake.call_count);
	EXPECT_EQ(0, CANIF_mob_enable_interrupt_fake.call_count);
}

TEST_F(CANDriver, TestAllExpectedFunctionsAreCalled)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;
	TransmitCAN(testPacket);
	EXPECT_EQ(1, CANIF_set_std_id_fake.call_count);
	EXPECT_EQ(1, CANIF_mob_clr_dlc_fake.call_count);
	EXPECT_EQ(1, CANIF_mob_set_dlc_fake.call_count);
	EXPECT_EQ(1, CANIF_set_data_fake.call_count);
	EXPECT_EQ(1, CANIF_config_tx_fake.call_count);
	EXPECT_EQ(1, CANIF_mob_enable_fake.call_count);
	EXPECT_EQ(1, CANIF_mob_enable_interrupt_fake.call_count);
}

TEST_F(CANDriver, TestTransmitCanBeCalled4Times)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;

	for(uint32_t i = 0; i < 4; i++)
	{
		EXPECT_EQ(NO_ERROR_CAN, TransmitCAN(testPacket));
	}
	EXPECT_EQ(ERROR_MOB_NOT_AVALIABLE, TransmitCAN(testPacket));
}

TEST_F(CANDriver, TestTransmitDoesntTransmitWhenNoMOBsAvailable)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;

	FILL_ALL_TX_MOBS;
	RESET_FAKE(CANIF_set_std_id);
	RESET_FAKE(CANIF_mob_clr_dlc);
	RESET_FAKE(CANIF_mob_set_dlc);
	RESET_FAKE(CANIF_set_data);
	RESET_FAKE(CANIF_config_tx);
	RESET_FAKE(CANIF_mob_enable);
	RESET_FAKE(CANIF_mob_enable_interrupt);

	TransmitCAN(testPacket);
	EXPECT_EQ(0, CANIF_set_std_id_fake.call_count);
	EXPECT_EQ(0, CANIF_mob_clr_dlc_fake.call_count);
	EXPECT_EQ(0, CANIF_mob_set_dlc_fake.call_count);
	EXPECT_EQ(0, CANIF_set_data_fake.call_count);
	EXPECT_EQ(0, CANIF_config_tx_fake.call_count);
	EXPECT_EQ(0, CANIF_mob_enable_fake.call_count);
	EXPECT_EQ(0, CANIF_mob_enable_interrupt_fake.call_count);
}

TEST_F(CANDriver, TestCorrectValuesArePassedInto_CANIF_set_std_id)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;
	testPacket.id = 0xF2;
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	EXPECT_EQ(0 , CANIF_set_std_id_fake.arg0_val);
	EXPECT_EQ(1 , CANIF_set_std_id_fake.arg1_val);
	EXPECT_EQ(0xF2 , CANIF_set_std_id_fake.arg2_val);
}

TEST_F(CANDriver, TestCorrectValuesArePassedInto_CANIF_mob_clr_dlc)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	EXPECT_EQ(0, CANIF_mob_clr_dlc_fake.arg0_val);
	EXPECT_EQ(2, CANIF_mob_clr_dlc_fake.arg1_val);
}

TEST_F(CANDriver, TestCorrectValuesArePassedInto_CANIF_mob_set_dlc)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	EXPECT_EQ(0, CANIF_mob_set_dlc_fake.arg0_val);
	EXPECT_EQ(2, CANIF_mob_set_dlc_fake.arg1_val);
	EXPECT_EQ(8, CANIF_mob_set_dlc_fake.arg2_val);
}

TEST_F(CANDriver, TestCorrectValuesArePassedInto_CANIF_set_data)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;
	testPacket.data[0] = 4;
	testPacket.data[1] = 3;
	testPacket.data[2] = 5;
	testPacket.data[3] = 6;
	testPacket.data[4] = 7;
	testPacket.data[5] = 8;
	testPacket.data[6] = 9;
	testPacket.data[7] = 10;
	testPacket.id = 0xF2;
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	EXPECT_EQ(0, CANIF_set_data_fake.arg0_val);
	EXPECT_EQ(2, CANIF_set_data_fake.arg1_val);
	EXPECT_EQ(0xA09080706050304, CANIF_set_data_fake.arg2_val);
}

TEST_F(CANDriver, TestCorrectValuesArePassedInto_CANIF_config_tx)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	EXPECT_EQ(0, CANIF_config_tx_fake.arg0_val);
	EXPECT_EQ(2, CANIF_config_tx_fake.arg1_val);
}

TEST_F(CANDriver, TestCorrectValuesArePassedInto_CANIF_mob_enable)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	EXPECT_EQ(0, CANIF_mob_enable_fake.arg0_val);
	EXPECT_EQ(2, CANIF_mob_enable_fake.arg1_val);
}

TEST_F(CANDriver, TestCorrectValuesArePassedInto_CANIF_mob_enable_interrupt)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	TransmitCAN(testPacket);
	EXPECT_EQ(0, CANIF_mob_enable_interrupt_fake.arg0_val);
	EXPECT_EQ(2, CANIF_mob_enable_interrupt_fake.arg1_val);
}

/***********************************************************/
/* Test TransmitOK ISR.							           */
/***********************************************************/
TEST_F(CANDriver, TestTransmitOKCallsCorrectFunctions)
{
	p_CANTransmitOK_ISR();

	EXPECT_EQ(1, CANIF_mob_get_mob_txok_fake.call_count);
	EXPECT_EQ(1, CANIF_mob_clear_txok_status_fake.call_count);
	EXPECT_EQ(1, CANIF_mob_clear_status_fake.call_count);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_mob_get_mob_txok)
{
	p_CANTransmitOK_ISR();
	EXPECT_EQ(0 ,CANIF_mob_get_mob_txok_fake.arg0_val);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_mob_clear_txok_status)
{
	CANIF_mob_get_mob_txok_fake.return_val = 0xFA;
	p_CANTransmitOK_ISR();
	EXPECT_EQ(0, CANIF_mob_clear_txok_status_fake.arg0_val);
	EXPECT_EQ(0xFA, CANIF_mob_clear_txok_status_fake.arg1_val);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_mob_clear_status)
{
	CANIF_mob_get_mob_txok_fake.return_val = 0xFA;
	p_CANTransmitOK_ISR();
	EXPECT_EQ(0, CANIF_mob_clear_status_fake.arg0_val);
	EXPECT_EQ(0xFA, CANIF_mob_clear_status_fake.arg1_val);
}

TEST_F(CANDriver, TestMOBsAreFreed)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;

	FILL_ALL_TX_MOBS;

	for(uint32_t i = 0; i < 4; i++)
	{
		EXPECT_EQ(ERROR_MOB_NOT_AVALIABLE, TransmitCAN(testPacket));
		FREE_MOB(i);
		EXPECT_EQ(NO_ERROR_CAN, TransmitCAN(testPacket));
	}
}

TEST_F(CANDriver, TestMOBFreeUpperBound)
{
	CAN_PACKET	testPacket;
	testPacket.dataSize = 8;

	FILL_ALL_TX_MOBS;
	FREE_MOB(9);
	EXPECT_EQ(ERROR_MOB_NOT_AVALIABLE, TransmitCAN(testPacket));
}

/***********************************************************/
/* Test ReceiveOK ISR.							           */
/***********************************************************/
TEST_F(CANDriver, TestReceiveOKCallsCorrectFunctions)
{
	p_CANReceiveOK_ISR();

	EXPECT_EQ(1, CANIF_mob_get_mob_rxok_fake.call_count);
	EXPECT_EQ(1, CANIF_mob_clear_rxok_status_fake.call_count);
	EXPECT_EQ(1, CANIF_mob_clear_status_fake.call_count);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_mob_get_mob_rxok)
{
	p_CANReceiveOK_ISR();
	EXPECT_EQ(0 ,CANIF_mob_get_mob_rxok_fake.arg0_val);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_mob_clear_rxok_status)
{
	CANIF_mob_get_mob_rxok_fake.return_val = 0xFA;
	p_CANReceiveOK_ISR();
	EXPECT_EQ(0, CANIF_mob_clear_rxok_status_fake.arg0_val);
	EXPECT_EQ(0xFA, CANIF_mob_clear_rxok_status_fake.arg1_val);
}

TEST_F(CANDriver, Receive_TestValuePassedInto_CANIF_mob_clear_status)
{
	CANIF_mob_get_mob_rxok_fake.return_val = 0xFA;
	p_CANReceiveOK_ISR();
	EXPECT_EQ(0, CANIF_mob_clear_status_fake.arg0_val);
	EXPECT_EQ(0xFA, CANIF_mob_clear_status_fake.arg1_val);
}

/***********************************************************/
/* Test ReceiveCAN   							           */
/***********************************************************/
TEST_F(CANDriver, CANReceiveRejectsNullPointer)
{
	uint16_t cobID = 1;
	InitialiseCANDriver(&cobID, 1);
	EXPECT_EQ(NULL_POINTER , ReceiveCAN(0));
}

TEST_F(CANDriver, CANReceiveReturnsNoPacketsWhenEmpty)
{
	uint16_t cobID = 1;
	InitialiseCANDriver(&cobID, 1);
	EXPECT_EQ(NO_PACKETS_RECEIVED, ReceiveCAN((CAN_PACKET *)1));
}

TEST_F(CANDriver, CANReceiveReturnsAPacketWhenItHasOne)
{
	CAN_PACKET packet;
	uint16_t cobID = 1;
	InitialiseCANDriver(&cobID, 1);
	CANIF_mob_get_mob_rxok_fake.return_val = 8;
	p_CANReceiveOK_ISR();
	EXPECT_EQ(NO_ERROR_CAN, ReceiveCAN(&packet));
}

TEST_F(CANDriver, CANReceiveReturnsCorrectPacketData)
{
	CAN_PACKET packet;
	uint16_t cobID = 1;
	InitialiseCANDriver(&cobID, 1);
	CANIF_mob_get_mob_rxok_fake.return_val = 8;
	CANIF_mob_get_dlc_fake.return_val = 8;
	p_messageObjectBufferChannel0[8].data.u64 = 0xEFCDAB8967452301;
	p_messageObjectBufferChannel0[8].id = 0x2468;
	p_CANReceiveOK_ISR();
	ReceiveCAN(&packet);
	
	EXPECT_EQ(0x01, packet.data[0]);
	EXPECT_EQ(0x23, packet.data[1]);
	EXPECT_EQ(0x45, packet.data[2]);
	EXPECT_EQ(0x67, packet.data[3]);
	EXPECT_EQ(0x89, packet.data[4]);
	EXPECT_EQ(0xAB, packet.data[5]);
	EXPECT_EQ(0xCD, packet.data[6]);
	EXPECT_EQ(0xEF, packet.data[7]);
	EXPECT_EQ(0x2468, packet.id);
	EXPECT_EQ(8, packet.dataSize);
}

TEST_F(CANDriver, CANReceiveReturnsNoPacketReceivedForInvalidDataLength)
{
	CAN_PACKET packet;
	uint16_t cobID = 1;
	InitialiseCANDriver(&cobID, 1);
	CANIF_mob_get_mob_rxok_fake.return_val = 8;
	CANIF_mob_get_dlc_fake.return_val = 9;
	p_messageObjectBufferChannel0[8].data.u64 = 0xEFCDAB8967452301;
	p_messageObjectBufferChannel0[8].id = 0x2468ACE0;
	p_CANReceiveOK_ISR();

	EXPECT_EQ(NO_PACKETS_RECEIVED, ReceiveCAN(&packet));
}

TEST_F(CANDriver, CANReceiveReturnsPacketsUpToBufferLength)
{
	CAN_PACKET packet;
	uint32_t i;
	uint16_t cobID = 1;
	InitialiseCANDriver(&cobID, 1);
	CANIF_mob_get_mob_rxok_fake.return_val = 8;
	CANIF_mob_get_dlc_fake.return_val = 8;
	p_messageObjectBufferChannel0[8].data.u64 = 0xEFCDAB8967452301;
	p_messageObjectBufferChannel0[8].id = 0x2468ACE0;
	for(i = 0; i < 11; i++) p_CANReceiveOK_ISR();
	
	for(i = 0; i < 10; i++)	EXPECT_EQ(NO_ERROR_CAN, ReceiveCAN(&packet));
	EXPECT_EQ(NO_PACKETS_RECEIVED, ReceiveCAN(&packet));
}

TEST_F(CANDriver, CANReceiveCorrectlyRecordsDroppedPackets)
{
	uint32_t i;
	uint16_t cobID = 1;
	InitialiseCANDriver(&cobID, 1);
	CANIF_mob_get_mob_rxok_fake.return_val = 8;
	CANIF_mob_get_dlc_fake.return_val = 8;
	for(i = 0; i < 20; i++) p_CANReceiveOK_ISR();
	
	EXPECT_EQ(10, PacketsDropped());
	ResetPacketsDroppedCount();
	EXPECT_EQ(0, PacketsDropped());
}

TEST_F(CANDriver, CANReceiveCorrectlyRecordsReceivedPackets)
{
	uint32_t i;
	uint16_t cobID = 1;
	InitialiseCANDriver(&cobID, 1);
	CANIF_mob_get_mob_rxok_fake.return_val = 8;
	CANIF_mob_get_dlc_fake.return_val = 8;
	for(i = 0; i < 10; i++) p_CANReceiveOK_ISR();
	
	EXPECT_EQ(10, PacketsReceived());
	ResetPacketsReceivedCount();
	EXPECT_EQ(0, PacketsReceived());
}

TEST_F(CANDriver, CANReceiveRepeatedlyConsumePackets)
{
	CAN_PACKET packet;
	uint32_t i;
	uint32_t j;
	uint16_t cobID = 1;
	InitialiseCANDriver(&cobID, 1);

	for(j = 0; j < 10000; j++)
	{
		for(i = 1; i < 8; i++)
		{
			CANIF_mob_get_mob_rxok_fake.return_val = 7 + i;
			CANIF_mob_get_dlc_fake.return_val = i;
			p_messageObjectBufferChannel0[7 + i].data.u8[0] = (uint8_t)i;
			p_messageObjectBufferChannel0[7 + i].id = i;
			p_CANReceiveOK_ISR();		
		}

		for(i = 1; i < 8; i++)
		{
			EXPECT_EQ(NO_ERROR_CAN, ReceiveCAN(&packet));
			EXPECT_EQ(i, packet.data[0]);
			EXPECT_EQ(i, packet.id);
			EXPECT_EQ(i, packet.dataSize);
		}
		EXPECT_EQ(NO_PACKETS_RECEIVED, ReceiveCAN(&packet));
	}	
}


TEST_F(CANDriver, TestCANHasTransmitSpaceAvailable)
{
	CAN_PACKET testPacket;
	testPacket.dataSize = 8;
	EXPECT_NE(0, CANHasTransmitSpaceAvailable());
	FILL_ALL_TX_MOBS;
	EXPECT_EQ(0, CANHasTransmitSpaceAvailable());
}


TEST_F(CANDriver, TestThatChangeChannelCallsAllTheRightFunctions)
{
	uint16_t cobID[11];
	ChangeCANChannel();
	EXPECT_EQ(1, CANIF_disable_interrupt_fake.call_count);
	EXPECT_EQ(2, CANIF_disable_fake.call_count);
	EXPECT_EQ(16, CANIF_mob_disable_interrupt_fake.call_count);
	EXPECT_EQ(16, CANIF_mob_disable_fake.call_count);
	EXPECT_EQ(2, CANIF_disable_fake.call_count);
	EXPECT_EQ(1, CANIF_set_reset_fake.call_count);
	EXPECT_EQ(2, CANIF_channel_enable_status_fake.call_count);
	EXPECT_EQ(1, CANIF_clr_reset_fake.call_count);
	EXPECT_EQ(1, CANIF_set_ram_add_fake.call_count);
	EXPECT_EQ(1, CANIF_set_sjw_fake.call_count);
	EXPECT_EQ(1, CANIF_set_prs_fake.call_count);
	EXPECT_EQ(1, CANIF_set_pres_fake.call_count);
	EXPECT_EQ(1, CANIF_set_phs2_fake.call_count);
	EXPECT_EQ(1, CANIF_set_phs1_fake.call_count);
	EXPECT_EQ(1, CANIF_set_channel_mode_fake.call_count);
	EXPECT_EQ(1, CANIF_clr_overrun_mode_fake.call_count);
	EXPECT_EQ(1, canif_clear_all_mob_fake.call_count);
	EXPECT_EQ(1, CANIF_enable_fake.call_count);
	EXPECT_EQ(1, CANIF_enable_interrupt_fake.call_count);
}

TEST_F(CANDriver, TestValuePassedInto_CANIF_set_std_id_OnChannelChange)
{
	uint16_t cobID[11] = {1,2,3,4,5,6,7,8,9,10,11};
	CANIF_channel_enable_status_fakeHistory[0] = 0;
	CANIF_channel_enable_status_fakeHistory[1] = 0;
	SET_RETURN_SEQ(CANIF_channel_enable_status, CANIF_channel_enable_status_fakeHistory, 2);
	InitialiseCANDriver(cobID, 11);
	ChangeCANChannel();
	EXPECT_EQ(1, CANIF_set_std_id_fake.arg2_history[0]);
	EXPECT_EQ(2, CANIF_set_std_id_fake.arg2_history[1]);
	EXPECT_EQ(3, CANIF_set_std_id_fake.arg2_history[2]);
	EXPECT_EQ(4, CANIF_set_std_id_fake.arg2_history[3]);
	EXPECT_EQ(5, CANIF_set_std_id_fake.arg2_history[4]);
	EXPECT_EQ(6, CANIF_set_std_id_fake.arg2_history[5]);
	EXPECT_EQ(7, CANIF_set_std_id_fake.arg2_history[6]);
	EXPECT_EQ(8, CANIF_set_std_id_fake.arg2_history[7]);
	EXPECT_EQ(9, CANIF_set_std_id_fake.arg2_history[8]);
	EXPECT_EQ(10, CANIF_set_std_id_fake.arg2_history[9]);
	EXPECT_EQ(11, CANIF_set_std_id_fake.arg2_history[10]);
}

TEST_F(CANDriver, TestChannelChangeActuallyChangesTheChannelValue)
{
	uint16_t cobID[11] = {1,2,3,4,5,6,7,8,9,10,11};
	CANIF_channel_enable_status_fakeHistory[0] = 0;
	CANIF_channel_enable_status_fakeHistory[1] = 0;
	SET_RETURN_SEQ(CANIF_channel_enable_status, CANIF_channel_enable_status_fakeHistory, 2);
	InitialiseCANDriver(cobID, 11);
	ChangeCANChannel();
	EXPECT_EQ(1, CANIF_set_sjw_fake.arg0_val);
	ChangeCANChannel();
	EXPECT_EQ(0, CANIF_set_sjw_fake.arg0_val);
}