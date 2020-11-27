#ifndef _MAILBOX_H
#define _MAILBOX_H

#include "avr32/io.h"
#include "compiler.h"
#include "can.h"
#include "board.h"


can_msg_t defaultTxMsg =
{
  {
    {
      .id = 0x000,                    // Identifier
      .id_mask  = 0x000,              // Mask
    },
  },
 .data.u64 = 0x00LL,    // Data
};

//Declaring the structure used to transmit messages with some predefined parameters
can_mob_t transmissionMessage[8] = {
                                    {
                                      CAN_MOB_NOT_ALLOCATED,
                                      &defaultTxMsg,
                                      8,
                                      CAN_DATA_FRAME,
                                      CAN_STATUS_NOT_COMPLETED
                                    },
                               };

can_msg_t defaultRxMsg =
{
  {
    {
      .id = 0,                      // Identifier
      .id_mask  = 0,                // Mask
    },
  },
 .data.u64 = 0x0LL,                 // Data
};

//Declaring the structure used to receive messages with some predefined parameters
can_mob_t receptionMessage[8] = {
									{	
										CAN_MOB_NOT_ALLOCATED,
										&defaultRxMsg,
										0,
										CAN_DATA_FRAME,
										CAN_STATUS_NOT_COMPLETED
									},
                                 };

#endif
