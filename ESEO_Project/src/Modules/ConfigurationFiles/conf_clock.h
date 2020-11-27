#ifndef CONF_CLOCK_H_INCLUDED
#define CONF_CLOCK_H_INCLUDED

#include "board.h"
#include "power_clocks_lib.h"

/* Clock source configuration parameters. */
//#define CONFIG_SYSCLK_SOURCE  SYSCLK_SRC_OSC0
#define CLK_SRC				PCL_MC_OSC0_PLL0
#define CLK_OSC0_F			BOARD_OSC0_HZ			//defined in board.h
#define CLK_OSC0_STARTUP	BOARD_OSC0_STARTUP_US	//defined in board.h

/* Max out all of our clocks */
#define CLK_CPU_F			60000000
#define CLK_PBA_F			60000000 
#define CLK_PBB_F			60000000 
#define CLK_PBC_F			60000000 
#define CLK_DFLL_F			60000000

/* Use the PLL0 to drive the clock up to 60MHz. */
#define CONFIG_SYSCLK_SOURCE      SYSCLK_SRC_PLL0
#define CONFIG_PLL0_MUL      10
#define CONFIG_PLL0_DIV       2

#endif
