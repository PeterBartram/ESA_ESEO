#ifndef CANIF_H
#define CANIF_H
#include <stdint.h>
#include "intc.h"

#define __attribute__(x)
#define __interrupt__	

#define AVR32_CANIF_ADDRESS                0xFFFD1C00
#define AVR32_CANIF_BUS_OFF_IRQ_0         288
#define AVR32_CANIF_BUS_OFF_IRQ_1         293
#define AVR32_CANIF_ERROR_IRQ_0           289
#define AVR32_CANIF_ERROR_IRQ_1           294
#define AVR32_CANIF_RXOK_IRQ_0            290
#define AVR32_CANIF_RXOK_IRQ_1            295
#define AVR32_CANIF_TXOK_IRQ_0            291
#define AVR32_CANIF_TXOK_IRQ_1            296
#define AVR32_CANIF_WAKE_UP_IRQ_0         292
#define AVR32_CANIF_WAKE_UP_IRQ_1         297
#define AVR32_CANIF_CAN_NB                 2
#define AVR32_CANIF_GCLK_NUM               1
#define AVR32_CANIF_RXLINE_1_PIN           1
#define AVR32_CANIF_RXLINE_1_FUNCTION      1
#define AVR32_CANIF_RXLINE_1_1_PIN         32
#define AVR32_CANIF_RXLINE_1_1_FUNCTION    1
#define AVR32_CANIF_RXLINE_0_PIN           36
#define AVR32_CANIF_RXLINE_0_FUNCTION      1
#define AVR32_CANIF_RXLINE_1_2_PIN         75
#define AVR32_CANIF_RXLINE_1_2_FUNCTION    1
#define AVR32_CANIF_RXLINE_0_1_PIN         85
#define AVR32_CANIF_RXLINE_0_1_FUNCTION    1
#define AVR32_CANIF_RXLINE_1_3_PIN         87
#define AVR32_CANIF_RXLINE_1_3_FUNCTION    1
#define AVR32_CANIF_RXLINE_0_2_PIN         105
#define AVR32_CANIF_RXLINE_0_2_FUNCTION    1
#define AVR32_CANIF_RXLINE_0_3_PIN         123
#define AVR32_CANIF_RXLINE_0_3_FUNCTION    1
#define AVR32_CANIF_RXLINE_0_4_PIN         48
#define AVR32_CANIF_RXLINE_0_4_FUNCTION    4
#define AVR32_CANIF_RXLINE_1_4_PIN         65
#define AVR32_CANIF_RXLINE_1_4_FUNCTION    4
#define AVR32_CANIF_RXLINE_1_5_PIN         79
#define AVR32_CANIF_RXLINE_1_5_FUNCTION    4
#define AVR32_CANIF_TXLINE_1_PIN           0
#define AVR32_CANIF_TXLINE_1_FUNCTION      1
#define AVR32_CANIF_TXLINE_1_1_PIN         33
#define AVR32_CANIF_TXLINE_1_1_FUNCTION    1
#define AVR32_CANIF_TXLINE_0_PIN           37
#define AVR32_CANIF_TXLINE_0_FUNCTION      1
#define AVR32_CANIF_TXLINE_1_2_PIN         76
#define AVR32_CANIF_TXLINE_1_2_FUNCTION    1
#define AVR32_CANIF_TXLINE_0_1_PIN         86
#define AVR32_CANIF_TXLINE_0_1_FUNCTION    1
#define AVR32_CANIF_TXLINE_1_3_PIN         88
#define AVR32_CANIF_TXLINE_1_3_FUNCTION    1
#define AVR32_CANIF_TXLINE_0_2_PIN         106
#define AVR32_CANIF_TXLINE_0_2_FUNCTION    1
#define AVR32_CANIF_TXLINE_0_3_PIN         124
#define AVR32_CANIF_TXLINE_0_3_FUNCTION    1
#define AVR32_CANIF_TXLINE_0_4_PIN         49
#define AVR32_CANIF_TXLINE_0_4_FUNCTION    4
#define AVR32_CANIF_TXLINE_1_4_PIN         64
#define AVR32_CANIF_TXLINE_1_4_FUNCTION    4
#define AVR32_CANIF_TXLINE_1_5_PIN         80
#define AVR32_CANIF_TXLINE_1_5_FUNCTION    4

#define CANIF_CHANNEL_MODE_NORMAL            0
#define CANIF_CHANNEL_MODE_LISTENING         1
#define CANIF_CHANNEL_MODE_LOOPBACK          2

//! 64-bit union.
typedef union
{
  int64_t s64   ;
  uint64_t u64   ;
  int32_t s32[2];
  uint64_t u32[2];
  int16_t s16[4];
  uint16_t u16[4];
  int8_t  s8 [8];
  uint8_t  u8 [8];
} Union64;

typedef struct
{
  union{
    struct{
      uint32_t   id          : 32;
      uint32_t   id_mask     : 32;
    };
    struct{
      uint32_t                   : 1;
      uint32_t   rtr_bit         : 1;
      uint32_t   ide_bit         : 1;
      uint32_t   id_bit          : 29;
      uint32_t                   : 1;
      uint32_t   rtr_mask_bit    : 1;
      uint32_t   ide_mask_bit    : 1;
      uint32_t   id_mask_bit     : 29;
    };
  };
 Union64 data;
} can_msg_t;


uint32_t CANIF_channel_enable_status(uint32_t);
void CANIF_disable(uint32_t);
void CANIF_set_reset(uint32_t);
void CANIF_clr_reset(uint32_t);
void CANIF_bit_timing(uint32_t);
void CANIF_clr_overrun_mode(uint32_t);
void CANIF_enable(uint32_t);
void CANIF_enable_interrupt(uint32_t);
uint32_t CANIF_mob_get_mob_txok(uint32_t);
void CANIF_mob_clear_txok_status(uint32_t, uint32_t);
void CANIF_mob_clear_status(uint32_t, uint32_t);
void CANIF_clr_interrupt_status(uint32_t);
void CANIF_set_channel_mode(uint32_t, uint32_t);
void canif_clear_all_mob(uint32_t, uint32_t);
void CANIF_clr_mob(uint32_t, uint32_t);
void CANIF_mob_clr_dlc(uint32_t, uint32_t);
void CANIF_config_tx(uint32_t, uint32_t);
void CANIF_mob_enable(uint32_t, uint32_t);
void CANIF_mob_enable_interrupt(uint32_t, uint32_t);
void CANIF_set_std_id(uint32_t, uint32_t, uint32_t);
void CANIF_mob_set_dlc(uint32_t, uint32_t, uint32_t);
void CANIF_set_data(uint32_t, uint32_t, uint64_t);
void CANIF_set_ram_add(uint32_t, unsigned long);
void CANIF_set_std_idmask(uint32_t, uint32_t, uint32_t);
void CANIF_config_rx(uint32_t,uint32_t);
uint32_t CANIF_mob_get_mob_rxok(uint32_t);
void CANIF_mob_clear_rxok_status(uint32_t, uint32_t);
uint32_t CANIF_mob_get_dlc(uint32_t, uint32_t);
uint16_t CANIF_get_ext_id(uint32_t, uint32_t);
void CANIF_set_sjw(uint32_t, uint32_t);
void CANIF_set_prs(uint32_t, uint32_t);
void CANIF_set_pres(uint32_t, uint32_t);
void CANIF_set_phs2(uint32_t, uint32_t);
void CANIF_set_phs1(uint32_t, uint32_t);
void CANIF_mob_disable_interrupt(uint32_t, uint32_t);
void CANIF_mob_disable(uint32_t, uint32_t);
void CANIF_disable_interrupt(uint32_t);

#endif