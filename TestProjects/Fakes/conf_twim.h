#ifndef _CONF_TWIM_H
#define _CONF_TWIM_H
	#if BOARD==UC3C_EK
	// These defines are missing from or wrong in the toolchain header file ip_xxx.h or part.h
	#	if UC3C
	#		if !defined(AVR32_TWIM0_GROUP)
	#			define AVR32_TWIM0_GROUP         25
	#		else
	#			warning "Duplicate define(s) to remove from the ASF"
	#		endif
	#		if !defined(AVR32_TWIM1_GROUP)
	#			define AVR32_TWIM1_GROUP         26
	#		else
	#			warning "Duplicate define(s) to remove from the ASF"
	#		endif
	#		if !defined(AVR32_TWIM2_GROUP)
	#			define AVR32_TWIM2_GROUP         45
	#		else
	#			warning "Duplicate define(s) to remove from the ASF"
	#		endif
	#	endif
	#	define CONF_TWIM_IRQ_LINE          AVR32_TWIM0_IRQ
	#	define CONF_TWIM_IRQ_GROUP         AVR32_TWIM0_GROUP
	#	define CONF_TWIM_IRQ_LEVEL         1
	#endif
#endif