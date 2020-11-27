
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "CANObjectDictionary.h"
#include "CANODInterface.h"
#include "OperatingModes.h"
#include "gpio.h"
#include "Downlink.h"
#include "dacout.h"
#include "gpio.h"

/* The GPIO pin numbers we are using. */
#define BPSK_ON_OFF_SWITCH				AVR32_PIN_PC17
#define TRANSPONDER_ON_OFF_SWITCH		AVR32_PIN_PC18
#define TRANSPONDER_POWER_SWITCH		AVR32_PIN_PB02
#define ANTENNA_SWITCH					AVR32_PIN_PC19
#define DECODER_TONE_INPUT				AVR32_PIN_PB28

/* Active low CTCSS tone. */
#define CTCSS_TONE_ACTIVE				0

#define PANEL_1_DETECT_ENABLED			0x01
#define PANEL_2_DETECT_ENABLED			0x02
#define PANEL_3_DETECT_ENABLED			0x04

#define TIMER_TICKS_TO_WAIT					5						// Time to wait for the timer action to reach the freertos thread.
#define TRACO_TEMPERATURE_CUT_OFF_VALUE_TELEMETRY		59			// Raw ADC value representing 50 degrees C on the Traco.
#define TRACO_TEMPERATURE_CUT_OFF_VALUE_RECEIVE_ONLY	10			// Raw ADC value representing 70 degrees C on the Traco.

/* Represents the possible RF modes we can be in. */
typedef enum _RF_MODE
{
	RECEIVE_ONLY = 0,
	LOW_POWER_TELEMETRY = 1,
	HIGH_POWER_TELEMETRY = 2,
	LOW_POWER_TRANSPONDER = 3,
	HIGH_POWER_TRANSPONDER = 4,
	AUTONOMOUS_MODE_ENABLE = 5
}RF_MODE;

/* Represents the possible data modes that we can be in. */
typedef enum _DATA_MODE_CAN_OD
{
	DATA_MODE_1K2 = 0,
	DATA_MODE_4K8 = 1
}DATA_MODE_CAN_OD;

typedef enum _AUTONOMOUS_MODE
{
	AUTONOMOUS_MODE_A = 0,
	AUTONOMOUS_MODE_B = 1
}AUTONOMOUS_MODE;

/* All of the operational data that we need in order to make an operational decision. */
typedef struct _OPERATIONAL_DATA
{
	uint32_t dataMode;
	uint32_t rfMode;
	uint32_t ctcssMode;
	uint32_t solarPanelValues[3];
	uint32_t solaPanelEclipseThreshold;
	uint32_t eclipsePanels;
	uint32_t autonomousMode;
	uint32_t  safeModeEnabled;
	uint32_t  safeModeActive;
	uint32_t  tracoTemperature;
}OPERATIONAL_DATA;

OPERATIONAL_DATA lastCycle;

#define DELAY_10S 10000
#define DELAY_20S (DELAY_10S * 2)
#define DELAY_5M  (DELAY_10S * 30)

static StaticTimer_t eclipseTimer;
static xTimerHandle  eclipseTimerHandle;
static StaticTimer_t ctcssLostTimer;
static xTimerHandle  ctcssLostTimerHandle;
static StaticTimer_t ctcssOperatingTimer;
static xTimerHandle  ctcssOperatingTimerHandle;
static StaticTimer_t safeModeTimer;
static xTimerHandle  safeModeTimerHandle;


/* Autonomous mode A variables. */
static uint8_t eclipseTimeExpiredFlag;
static uint8_t weWereInEclipseLastTime;
static uint8_t weAreOfficialyInEclipseAutonomousModeA;	// We have been in eclipse long enough to be operating as if we are in eclipse.
static uint8_t transponderIsActive;					

/* Autonomous mode B variables. */
static uint8_t weAreInTransponderMode;

/* Private functions. */
static void IntialiseGPIO(OPERATIONAL_DATA * z_operationalData);
static void InitialiseTransmissionMode(OPERATIONAL_DATA z_operationalData);
static void GetOperationalData(OPERATIONAL_DATA * z_operationalData);
static void UpdateAutonomousModes(OPERATIONAL_DATA z_operationalData, OPERATIONAL_DATA z_operationalDataLastTime);
static void UpdateAutonomousModeA(OPERATIONAL_DATA z_operationalData);
static uint8_t UpdateAutonomousModeAEclipseDetection(OPERATIONAL_DATA z_operationalData);
static void UpdateAutonomousModeAEclipseModeOperations(OPERATIONAL_DATA z_operationalData);
static void UpdateAutonomousModeB(OPERATIONAL_DATA z_operationalData);
static uint32_t GetSolarPanelAverage(OPERATIONAL_DATA z_operationalData);
static void eclipseTimerExpiredCallback(xTimerHandle xTimer);
static void ctcssLostCallback(xTimerHandle xTimer);
static void SafeModeCallback(xTimerHandle xTimer);
static void ctcssOperatingCallback(xTimerHandle xTimer);
static void ChangeToLowPowerTelemetry1k2Mode(void);
static void ChangeToLowPowerTelemetry4k8Mode(void);
static void ChangeToHighPowerTelemetry1k2Mode(void);
static void ChangeToHighPowerTelemetry4k8Mode(void);
static void ChangeToTransponderLowPower(void);
static void ChangeToTransponderHighPower(void);
static void DisableTransponder(void);
static uint8_t InSafeMode(OPERATIONAL_DATA * z_data);

static uint8_t safeModeActiveLastTime;

void InitialiseOperatingModes(void)
{
	OPERATIONAL_DATA operationalData;
	IntialiseGPIO(&operationalData);

	InitialiseTransmissionMode(operationalData);
	lastCycle.dataMode = operationalData.dataMode;
	lastCycle.rfMode = operationalData.rfMode;
	lastCycle.autonomousMode = operationalData.autonomousMode;

	/* Initialise Autonomous mode A */
	eclipseTimerHandle = xTimerCreateStatic("EclipseTimer", DELAY_10S, 0, 0, &eclipseTimerExpiredCallback, &eclipseTimer);
	weAreOfficialyInEclipseAutonomousModeA = 1;
	weWereInEclipseLastTime = 0;
	eclipseTimeExpiredFlag = 0;
	transponderIsActive = 0;

	/* Initialise Autonomous mode B */
	ctcssLostTimerHandle = xTimerCreateStatic("CTCSSLost", DELAY_20S, 0, 0, &ctcssLostCallback, &ctcssLostTimer);
	ctcssOperatingTimerHandle = xTimerCreateStatic("CTCSSOperating", DELAY_5M, 0, 0, &ctcssOperatingCallback, &ctcssOperatingTimer);
	weAreInTransponderMode = 0;

	/* Initialise safe mode hysteresis timer */
	safeModeTimerHandle = xTimerCreateStatic("SafeModeTimer", DELAY_20S, 0, 0, &SafeModeCallback, &safeModeTimer);
	safeModeActiveLastTime = 0;
}

/*
This isn't particularly elegant but essential to make sure we dont have 
the GPIO configured wrong even for a short time on start up - reflected RF power = bad. 
*/
static void IntialiseGPIO(OPERATIONAL_DATA * z_operationalData)
{
	uint32_t bpskSwitch = GPIO_DIR_OUTPUT;
	uint32_t transponderSwitch = GPIO_DIR_OUTPUT;
	uint32_t transponderPowerSwitch = GPIO_DIR_OUTPUT;
	uint32_t antennaSwitch = GPIO_DIR_OUTPUT;

	if(z_operationalData)
	{
		/* Get the subset of operational data that we need to determine behaviour on start up. */
		GetCANObjectDataRaw(AMS_OBC_RFMODE, &z_operationalData->rfMode);
		GetCANObjectDataRaw(AMS_OBC_DATAMODE, &z_operationalData->dataMode);
		GetCANObjectDataRaw(AMS_OBC_CTCSS_MODE, &z_operationalData->ctcssMode);
		GetCANObjectDataRaw(AMS_OBC_ECLIPSE_MODE, &z_operationalData->autonomousMode);

		/* Need to confiured differently based upon the RF mode. */
		switch(z_operationalData->rfMode)
		{
			case RECEIVE_ONLY:
				break;
				case LOW_POWER_TELEMETRY:
					bpskSwitch |= GPIO_INIT_HIGH;				// Telemtry, BPSK enabled, BPSK antenna switch.
				break;
				case HIGH_POWER_TELEMETRY:
					bpskSwitch |= GPIO_INIT_HIGH;				// Telemtry, BPSK enabled, BPSK antenna switch.
				break;
				case LOW_POWER_TRANSPONDER:
					transponderSwitch |= GPIO_INIT_HIGH;		// Transponder, FM enabled, FM antenna switch.
					antennaSwitch |= GPIO_INIT_HIGH;
				break;
				case HIGH_POWER_TRANSPONDER:
					transponderSwitch |= GPIO_INIT_HIGH;		// Transponder, FM enabled, FM antenna switch.
					transponderPowerSwitch |= GPIO_INIT_HIGH;
					antennaSwitch |= GPIO_INIT_HIGH;
				break;
				case AUTONOMOUS_MODE_ENABLE:
					/* Switch based upon which autonoumous mode we are in */
					switch(z_operationalData->autonomousMode)
					{
					case AUTONOMOUS_MODE_A:
						/* Assume we are in eclipse for now. */
						transponderSwitch |= GPIO_INIT_HIGH;		// Transponder, FM enabled, FM antenna switch.
						antennaSwitch |= GPIO_INIT_HIGH;
						break;
					case AUTONOMOUS_MODE_B:
						/* Assume we haven't received a CTCSS tone for now. */
						bpskSwitch |= GPIO_INIT_HIGH;				// Telemtry, BPSK enabled, BPSK antenna switch.
						break;
					default:
						/* Just ignore this */
						break;
					}
				break;
				default:

					break;
		}

		/* Configure the GPIO pins */
		gpio_configure_pin(BPSK_ON_OFF_SWITCH, bpskSwitch);
		gpio_configure_pin(TRANSPONDER_ON_OFF_SWITCH, transponderSwitch);
		gpio_configure_pin(TRANSPONDER_POWER_SWITCH, transponderPowerSwitch);
		gpio_configure_pin(ANTENNA_SWITCH, antennaSwitch);
		gpio_configure_pin(DECODER_TONE_INPUT, GPIO_DIR_INPUT);
	}
}

/* Initialy configure the transmission mode based upon the default values from flash. */
static void InitialiseTransmissionMode(OPERATIONAL_DATA z_operationalData)
{
	switch(z_operationalData.rfMode)
	{
		case RECEIVE_ONLY:
			DisableTransponder();
			break;
		case LOW_POWER_TELEMETRY:
			/* Change between 1k2 and 4k8 mode at low power */
			if(z_operationalData.dataMode == DATA_MODE_1K2)
			{
				ChangeToLowPowerTelemetry1k2Mode();
			}
			else if(z_operationalData.dataMode == DATA_MODE_4K8)
			{
				ChangeToLowPowerTelemetry4k8Mode();
			}
				
			break;
		case HIGH_POWER_TELEMETRY:
			/* Change between 1k2 and 4k8 mode at high power */
			if(z_operationalData.dataMode == DATA_MODE_1K2)
			{
				ChangeToHighPowerTelemetry1k2Mode();
			}
			else if(z_operationalData.dataMode == DATA_MODE_4K8)
			{
				ChangeToHighPowerTelemetry4k8Mode();
			}
			break;
		case LOW_POWER_TRANSPONDER:
			ChangeToTransponderLowPower();
			break;
		case HIGH_POWER_TRANSPONDER:
			ChangeToTransponderHighPower();
			break;
		case AUTONOMOUS_MODE_ENABLE:
		{
			/* Switch based upon which autonoumous mode we are in */
			switch(z_operationalData.autonomousMode)
			{
			case AUTONOMOUS_MODE_A:
				ChangeToTransponderHighPower();
				break;
			case AUTONOMOUS_MODE_B:
				ChangeToHighPowerTelemetry1k2Mode();
			default:
				/* An error has occurred */
				break;
			}
		}
	}
}

/* Perform temperature checking and action this accordingly. */
static uint8_t InSafeMode(OPERATIONAL_DATA * z_data)
{
	uint8_t retVal = 0;

	if(z_data)
	{
		if(z_data->tracoTemperature < TRACO_TEMPERATURE_CUT_OFF_VALUE_RECEIVE_ONLY)
		{
			DisableTransponder();
			SetCANObjectDataRaw(AMS_OBC_SAFE_MODE_ACTIVE, 1);
			if(safeModeTimerHandle != 0)
			{
				xTimerStop(safeModeTimerHandle, TIMER_TICKS_TO_WAIT);
			}
			safeModeActiveLastTime = 1;
			retVal = 1;
		}
		else if(z_data->tracoTemperature < TRACO_TEMPERATURE_CUT_OFF_VALUE_TELEMETRY)
		{
			if(z_data->safeModeEnabled)
			{
				ChangeToLowPowerTelemetry1k2Mode();
				SetCANObjectDataRaw(AMS_OBC_SAFE_MODE_ACTIVE, 1);
				if(safeModeTimerHandle != 0)
				{
					xTimerStop(safeModeTimerHandle, TIMER_TICKS_TO_WAIT);
				}
				safeModeActiveLastTime = 1;
				retVal = 1;
			}
		}
		else
		{
			if(safeModeActiveLastTime)
			{
				if(safeModeTimerHandle != 0)
				{
					xTimerReset(safeModeTimerHandle, TIMER_TICKS_TO_WAIT);
				}
			}
			else
			{
				if(safeModeTimerHandle != 0)
				{
					/* If our hysteresis condition has been met. */
					if(!xTimerIsTimerActive(safeModeTimerHandle))
					{
						SetCANObjectDataRaw(AMS_OBC_SAFE_MODE_ACTIVE, 0);						
						retVal = 0;
					}
				}
			}
			safeModeActiveLastTime = 0;
		}
	}

	return retVal;
}

/* Detect changes in RF and data modes and change operating mode appropriately. */
void UpdateOperatingModes(void)
{
	OPERATIONAL_DATA operationalData;
	GetOperationalData(&operationalData);


	if(!InSafeMode(&operationalData))
	{
		/* If any of our modes have changed then we need to switch modes. */
		if((operationalData.dataMode != lastCycle.dataMode) || (operationalData.rfMode != lastCycle.rfMode) ||
		   (operationalData.autonomousMode != lastCycle.autonomousMode))
		{
			InitialiseTransmissionMode(operationalData);
		}

		/* See if our autonomous mode behaviour needs to be updated. */
		UpdateAutonomousModes(operationalData, lastCycle);
	}
	/* Save all of our values for the next time we are called. */
	lastCycle.dataMode = operationalData.dataMode;
	lastCycle.rfMode = operationalData.rfMode;
	lastCycle.ctcssMode = operationalData.ctcssMode;
	lastCycle.eclipsePanels = operationalData.eclipsePanels;
	lastCycle.solaPanelEclipseThreshold = operationalData.solaPanelEclipseThreshold;
	lastCycle.autonomousMode = operationalData.autonomousMode;
}

/* Responsible for resetting anutonomous modes when we are placed into auto mode and also for updateing the mode itself. */
static void UpdateAutonomousModes(OPERATIONAL_DATA z_operationalData, OPERATIONAL_DATA z_operationalDataLastTime)
{
	/* Are we in an autonomous mode? */
	if(z_operationalData.rfMode == AUTONOMOUS_MODE_ENABLE)
	{
		/* If we werent in autonomous mode last time or we have changed modes then we need to initialise again */
		if((z_operationalData.rfMode == AUTONOMOUS_MODE_ENABLE && z_operationalDataLastTime.rfMode != AUTONOMOUS_MODE_ENABLE) || 
			(z_operationalData.autonomousMode != z_operationalDataLastTime.autonomousMode))
		{
			/* Reset our object variables */
			weAreOfficialyInEclipseAutonomousModeA = 1;
			weWereInEclipseLastTime = 0;
			eclipseTimeExpiredFlag = 0;
			transponderIsActive = 0;

			/* Clear all of our timers when we change modes. */
			if(eclipseTimerHandle != 0)
			{
				xTimerStop(eclipseTimerHandle, TIMER_TICKS_TO_WAIT);
			}

			if(ctcssLostTimerHandle != 0)
			{
				xTimerStop(ctcssLostTimerHandle, TIMER_TICKS_TO_WAIT);
			}

			if(ctcssOperatingTimerHandle != 0)
			{
				xTimerStop(ctcssOperatingTimerHandle, TIMER_TICKS_TO_WAIT);
			}
		}

		if(z_operationalData.autonomousMode == AUTONOMOUS_MODE_A)
		{
			UpdateAutonomousModeA(z_operationalData);
		}
		else if(z_operationalData.autonomousMode == AUTONOMOUS_MODE_B)
		{
			UpdateAutonomousModeB(z_operationalData);
		}
		else
		{
			/* Error occurred - ignore */
		}
	}
}

static void UpdateAutonomousModeA(OPERATIONAL_DATA z_operationalData)
{
	uint8_t weAreInEclipse;

	weAreInEclipse = UpdateAutonomousModeAEclipseDetection(z_operationalData);
	UpdateAutonomousModeAEclipseModeOperations(z_operationalData);

	/* Record the sunlight situation for next time. */
	weWereInEclipseLastTime = weAreInEclipse;
}

/* This function looks for the change from eclipse to sunlight and vice versa when
   we are in autonomous mode A and returns a value for whether or not we are in eclipse. */
static uint8_t UpdateAutonomousModeAEclipseDetection(OPERATIONAL_DATA z_operationalData)
{
	uint8_t weAreInEclipse = 0;

	if(eclipseTimerHandle != 0)
	{
		/* Are we in eclipse ? */
		if(GetSolarPanelAverage(z_operationalData) < z_operationalData.solaPanelEclipseThreshold)
		{
			weAreInEclipse = 1;
		}

		/* Has our sunlight situation changed? */
		if(weAreInEclipse != weWereInEclipseLastTime)
		{
			eclipseTimeExpiredFlag = 0;
			xTimerReset(eclipseTimerHandle, TIMER_TICKS_TO_WAIT);
		}
		else
		{
			/* If our timer has expired. */
			if(eclipseTimeExpiredFlag != 0)
			{
				eclipseTimeExpiredFlag = 0;

				if(weAreInEclipse)
				{
					if(z_operationalData.ctcssMode != 0)
					{
						if(gpio_get_pin_value(DECODER_TONE_INPUT) == CTCSS_TONE_ACTIVE)
						{
							transponderIsActive = 1;
							ChangeToTransponderHighPower();
						}
						else
						{
							transponderIsActive = 0;
						}
					}
					else
					{
						/* Have been in eclipse for 10 seconds so change into transponder mode. */
						ChangeToTransponderHighPower();
						transponderIsActive = 1;
					}
					weAreOfficialyInEclipseAutonomousModeA = 1;
				}
				else
				{
					/* Have been out of eclipse for 10 seconds so change into telemetry mode. */
					ChangeToHighPowerTelemetry1k2Mode();
					weAreOfficialyInEclipseAutonomousModeA = 0;
				}
			}
		}
	}
	return weAreInEclipse;
}

/* Updat autonomous mode A based upn our knowledge of being in eclipse. */
static void UpdateAutonomousModeAEclipseModeOperations(OPERATIONAL_DATA z_operationalData)
{
	/* Need to switch transponder on and off when in eclipse */
	if(weAreOfficialyInEclipseAutonomousModeA)
	{
		/* If CTCSS mode is active then we need to look for CTCSS tone. */
		if(z_operationalData.ctcssMode != 0)
		{
			/* Have we received a tone? */
			if(gpio_get_pin_value(DECODER_TONE_INPUT) == CTCSS_TONE_ACTIVE)
			{
				/* Don't constantly attempt to turn on the transponder. */
				if(!transponderIsActive)
				{
					/* Switch on the transponder. */
					ChangeToTransponderHighPower();
					transponderIsActive = 1;
				}
			}
			else
			{
				/* Disable the transponder as we dont have a tone. */
				DisableTransponder();
				transponderIsActive = 0;
			}
		}
		else
		{
			/* Don't constantly attempt to turn on the transponder. */
			if(!transponderIsActive)
			{
				ChangeToTransponderHighPower();
				transponderIsActive = 1;
			}
		}
	}
}

/* Perform autonomous mode B operations. */
static void UpdateAutonomousModeB(OPERATIONAL_DATA z_operationalData)
{
	uint8_t ctcssToneDetected = 0;
	
	/* Get our CTCSS tone pin value. */
	ctcssToneDetected = gpio_get_pin_value(DECODER_TONE_INPUT);
	
	if(weAreInTransponderMode)
	{
		/* CTCSS tone is being detected by the RF PCB. */
		if(ctcssToneDetected == CTCSS_TONE_ACTIVE)
		{
			/* If our 5 minute timer has expired. */
			if(!xTimerIsTimerActive(ctcssOperatingTimerHandle))
			{
				/* Switch to telemetry mode. */
				ChangeToHighPowerTelemetry1k2Mode();
				weAreInTransponderMode = 0;
				/* Start a cooldown timer to stop us going back into transponder mode for 5 mins */
				xTimerReset(ctcssOperatingTimerHandle, TIMER_TICKS_TO_WAIT);
				/* If we time out during a period that CTCSS tone is missing then stop the timer. */
				xTimerStop(ctcssLostTimerHandle, TIMER_TICKS_TO_WAIT);
			}
		}
		else
		{
			if(!xTimerIsTimerActive(ctcssLostTimerHandle))
			{
				/* Switch to telemetry mode. */
				ChangeToHighPowerTelemetry1k2Mode();
				weAreInTransponderMode = 0;
				/* Start a cooldown timer to stop us going back into transponder mode for 5 mins */
				xTimerReset(ctcssOperatingTimerHandle, TIMER_TICKS_TO_WAIT);
			}
		}
	}
	else
	{
		/* CTCSS tone is being detected by the RF PCB. */
		if(ctcssToneDetected == CTCSS_TONE_ACTIVE)
		{
			if(!xTimerIsTimerActive(ctcssOperatingTimerHandle))
			{
				/* Switch to transponder mode here. */
				ChangeToTransponderHighPower();
				weAreInTransponderMode = 1;
				/* Start a timer for 5 minutes maximum to be spent in transponder mode. */
				xTimerReset(ctcssOperatingTimerHandle, TIMER_TICKS_TO_WAIT);
			}
		}
	}
}

static uint32_t GetSolarPanelAverage(OPERATIONAL_DATA z_operationalData)
{
	uint32_t sunlightValue = 0;
	uint8_t panelsEnabled = 0;

	/* Add in the enabled panel values */
	if(z_operationalData.eclipsePanels & PANEL_1_DETECT_ENABLED)
	{
		sunlightValue += z_operationalData.solarPanelValues[0];
		panelsEnabled++;
	}
	if(z_operationalData.eclipsePanels & PANEL_2_DETECT_ENABLED)
	{
		sunlightValue += z_operationalData.solarPanelValues[1];
		panelsEnabled++;
	}
	if(z_operationalData.eclipsePanels & PANEL_3_DETECT_ENABLED)
	{
		sunlightValue += z_operationalData.solarPanelValues[2];
		panelsEnabled++;
	}
	/* Take the average of the enabled panels. */
	if(panelsEnabled != 0)
	{
		sunlightValue /= panelsEnabled;
	}
	return sunlightValue;
}

static void ChangeToLowPowerTelemetry1k2Mode(void)
{
	ChangeDownlinkMode(MODE_1K2);
	ChangeMode(MODE_1K2_LOW_POWER);
	gpio_set_pin_high(BPSK_ON_OFF_SWITCH);
	gpio_set_pin_low(TRANSPONDER_ON_OFF_SWITCH);									
	gpio_set_pin_low(TRANSPONDER_POWER_SWITCH);
	gpio_set_pin_low(ANTENNA_SWITCH);

}
static void ChangeToLowPowerTelemetry4k8Mode(void)
{
	ChangeDownlinkMode(MODE_4K8);
	ChangeMode(MODE_4K8_LOW_POWER);
	gpio_set_pin_high(BPSK_ON_OFF_SWITCH);
	gpio_set_pin_low(TRANSPONDER_ON_OFF_SWITCH);									
	gpio_set_pin_low(TRANSPONDER_POWER_SWITCH);
	gpio_set_pin_low(ANTENNA_SWITCH);
}
static void ChangeToHighPowerTelemetry1k2Mode(void)
{
	ChangeDownlinkMode(MODE_1K2);
	ChangeMode(MODE_1K2_HIGH_POWER);
	gpio_set_pin_high(BPSK_ON_OFF_SWITCH);
	gpio_set_pin_low(TRANSPONDER_ON_OFF_SWITCH);									
	gpio_set_pin_low(TRANSPONDER_POWER_SWITCH);
	gpio_set_pin_low(ANTENNA_SWITCH);
}
static void ChangeToHighPowerTelemetry4k8Mode(void)
{
	ChangeDownlinkMode(MODE_4K8);
	ChangeMode(MODE_4K8_HIGH_POWER);
	gpio_set_pin_high(BPSK_ON_OFF_SWITCH);
	gpio_set_pin_low(TRANSPONDER_ON_OFF_SWITCH);									
	gpio_set_pin_low(TRANSPONDER_POWER_SWITCH);
	gpio_set_pin_low(ANTENNA_SWITCH);
}
static void ChangeToTransponderLowPower(void)
{
	ChangeDownlinkMode(MODE_RX_ONLY);
	gpio_set_pin_low(BPSK_ON_OFF_SWITCH);
	gpio_set_pin_high(TRANSPONDER_ON_OFF_SWITCH);									
	gpio_set_pin_low(TRANSPONDER_POWER_SWITCH);
	gpio_set_pin_high(ANTENNA_SWITCH);
}
static void ChangeToTransponderHighPower(void)
{
	ChangeDownlinkMode(MODE_RX_ONLY);
	gpio_set_pin_low(BPSK_ON_OFF_SWITCH);
	gpio_set_pin_high(TRANSPONDER_ON_OFF_SWITCH);									
	gpio_set_pin_high(TRANSPONDER_POWER_SWITCH);
	gpio_set_pin_high(ANTENNA_SWITCH);
}
static void DisableTransponder(void)
{
	ChangeDownlinkMode(MODE_RX_ONLY);
	gpio_set_pin_low(BPSK_ON_OFF_SWITCH);
	gpio_set_pin_low(TRANSPONDER_ON_OFF_SWITCH);									
	gpio_set_pin_high(TRANSPONDER_POWER_SWITCH);
	gpio_set_pin_high(ANTENNA_SWITCH);
}

static void GetOperationalData(OPERATIONAL_DATA * z_operationalData)
{
	if(z_operationalData != 0)
	{
		/* Get all of the information we need to determine operational behaviour. */
		GetCANObjectDataRaw(AMS_OBC_RFMODE, &z_operationalData->rfMode);
		GetCANObjectDataRaw(AMS_OBC_DATAMODE, &z_operationalData->dataMode);
		GetCANObjectDataRaw(AMS_OBC_CTCSS_MODE, &z_operationalData->ctcssMode);
		GetCANObjectDataRaw(AMS_OBC_ECLIPSE_MODE, &z_operationalData->autonomousMode);
		GetCANObjectDataRaw(AMS_OBC_ECLIPSE_PANELS, &z_operationalData->eclipsePanels);
		GetCANObjectDataRaw(PPM_VOLTAGE_SP1, &z_operationalData->solarPanelValues[0]);
		GetCANObjectDataRaw(PPM_VOLTAGE_SP2, &z_operationalData->solarPanelValues[1]);
		GetCANObjectDataRaw(PPM_VOLTAGE_SP3, &z_operationalData->solarPanelValues[2]);
		GetCANObjectDataRaw(AMS_OBC_ECLIPSE_THRESH, &z_operationalData->solaPanelEclipseThreshold);
		GetCANObjectDataRaw(AMS_OBC_AUTO_SAFE_MODE, &z_operationalData->safeModeEnabled);
		GetCANObjectDataRaw(AMS_OBC_SAFE_MODE_ACTIVE, &z_operationalData->safeModeActive);
		GetCANObjectDataRaw(AMS_EPS_DCDC_T, &z_operationalData->tracoTemperature);
	}
}

/* Callback function for when our eclipse changeover timer has expired. */
static void eclipseTimerExpiredCallback(xTimerHandle xTimer)
{
	eclipseTimeExpiredFlag = 1;
}

/* Unused callback function */
static void ctcssLostCallback(xTimerHandle xTimer)
{
}

/* Unused callback function */
static void ctcssOperatingCallback(xTimerHandle xTimer)
{
}
/* Unused callback function */
static void SafeModeCallback(xTimerHandle xTimer)
{
}