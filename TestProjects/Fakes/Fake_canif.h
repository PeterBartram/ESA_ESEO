#ifndef _FAKE_CANIF_H
#define _FAKE_CANIF_H

#include "canif.h"
#include "fff.h"

DECLARE_FAKE_VOID_FUNC1(CANIF_disable, uint32_t);
DECLARE_FAKE_VOID_FUNC1(CANIF_set_reset, uint32_t);
DECLARE_FAKE_VALUE_FUNC1(uint32_t, CANIF_channel_enable_status, uint32_t);
DECLARE_FAKE_VOID_FUNC1(CANIF_clr_reset, uint32_t);
DECLARE_FAKE_VOID_FUNC1(CANIF_bit_timing, uint32_t);
DECLARE_FAKE_VOID_FUNC1(CANIF_clr_overrun_mode, uint32_t);
DECLARE_FAKE_VOID_FUNC1(CANIF_enable, uint32_t);
DECLARE_FAKE_VOID_FUNC1(CANIF_enable_interrupt, uint32_t);
DECLARE_FAKE_VALUE_FUNC1(uint32_t, CANIF_mob_get_mob_txok, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_mob_clear_txok_status, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_mob_clear_status, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC1(CANIF_clr_interrupt_status, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_set_channel_mode, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(canif_clear_all_mob, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_clr_mob, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_mob_clr_dlc, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_config_tx, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_mob_enable, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_mob_enable_interrupt, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC3(CANIF_set_std_id, uint32_t, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC3(CANIF_mob_set_dlc, uint32_t, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC3(CANIF_set_data, uint32_t, uint32_t, uint64_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_set_ram_add, uint32_t, unsigned long);
DECLARE_FAKE_VOID_FUNC2(CANIF_config_rx, uint32_t,uint32_t);
DECLARE_FAKE_VOID_FUNC3(CANIF_set_std_idmask, uint32_t, uint32_t, uint32_t);
DECLARE_FAKE_VALUE_FUNC1(uint32_t, CANIF_mob_get_mob_rxok, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_mob_clear_rxok_status, uint32_t, uint32_t);
DECLARE_FAKE_VALUE_FUNC2(uint32_t, CANIF_mob_get_dlc,uint32_t, uint32_t);
DECLARE_FAKE_VALUE_FUNC2(uint16_t, CANIF_get_ext_id,uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_set_sjw, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_set_prs, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_set_pres, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_set_phs2, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_set_phs1, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_mob_disable_interrupt, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC2(CANIF_mob_disable, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC1(CANIF_disable_interrupt, uint32_t);

#endif