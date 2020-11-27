
//headers
#include "asf.h"
#include "compiler.h"
#include "board.h"
#include "gpio.h"
#include "pm_uc3c.h"
#include "scif_uc3c.h"		// System Control Interface (SCIF) - Used for handling PLLs and Generic Clocks for Modules
#include "intc.h"
#include <sysclk.h>
#include "Diagnostics.h"
#include "baseTimer.h"
#include "modulator.h"
#include "delay.h"
#include "CANOpen.h"
#include "power_clocks_lib.h"
#include "demodulator.h"
#include "Uplink.h"
#include "PayloadData.h"
#include "Downlink.h"
#include "SatelliteOperations.h"

/* RTOS header files. */
#include "FreeRTOS.h"
#include "task.h"

/* Temporary */
#include "usartDriver.h"
#include "queue.h"

#define TELEMETRY_BUFFER 100
uint16_t buffer[TELEMETRY_BUFFER];

/* Diagnostics configuration */
#define DIAGNOSTICS_STACK_SIZE configMINIMAL_STACK_SIZE
StackType_t diagnosticsStack[DIAGNOSTICS_STACK_SIZE];
StaticTask_t diagnosticsControlStructure;
xTaskHandle diagnosticsTaskHandle = 0;
#define DIAGNOSTICS_PRIORITY 4

/* CANOpen configuration */
#define CANOPEN_STACK_SIZE configMINIMAL_STACK_SIZE
StackType_t canOpenStack[CANOPEN_STACK_SIZE];
StaticTask_t canOpenControlStructure;
xTaskHandle canOpenTaskHandle = 0;
#define CAN_PRIORITY 3

/* Downlink configuration */
#define DOWNLINK_STACK_SIZE configMINIMAL_STACK_SIZE
StackType_t downlinkStack[DOWNLINK_STACK_SIZE];
StaticTask_t downlinkControlStructure;
xTaskHandle downlinkTaskHandle = 0;
#define DOWNLINK_PRIORITY 2

/* Uplink configuration */
#define UPLINK_STACK_SIZE configMINIMAL_STACK_SIZE
StackType_t uplinkStack[UPLINK_STACK_SIZE];
StaticTask_t uplinkControlStructure;
xTaskHandle uplinkTaskHandle = 0;
#define UPLINK_PRIORITY 2

/* Satellite Operations */
#define OPS_STACK_SIZE configMINIMAL_STACK_SIZE
StackType_t opsStack[UPLINK_STACK_SIZE];
StaticTask_t opsControlStructure;
xTaskHandle opsTaskHandle = 0;
#define OPS_PRIORITY 2


/* Payload Data */
#define PAYLOAD_DATA_STACK_SIZE configMINIMAL_STACK_SIZE
StackType_t payloadDataStack[3][UPLINK_STACK_SIZE];
StaticTask_t payloadDataControlStructure;
xTaskHandle payloadDataTaskHandle = 0;
#define PAYLOAD_DATA_PRIORITY 3

/* Idle task configuration */
StaticTask_t idleControlStructure;
StackType_t idleStack[configMINIMAL_STACK_SIZE];

/* Timer task configuration */
StaticTask_t timerControlStructure;
StackType_t timerStack[configMINIMAL_STACK_SIZE];

static void InitClock(void);

int main (void)
{	
	CAN_PACKET packet;
	QueueHandle_t payloadDataQueue = 0;
	SemaphoreHandle_t flashProtectionSemaphore = 0;
	
	/* These must be done before all other initalisation */
	/******************************************************/
	//vTraceInitTraceData();
	sysclk_init(); 
	InitClock();
	INTC_init_interrupts();
	InitialisePayloadData();
	payloadDataQueue = GetPayloadDataQueueHandle();					// Queue needs to be initialised before other tasks. 
	InitialiseCANOpen(payloadDataQueue);							// Queue needs to be initialised before other tasks. 
	flashProtectionSemaphore = InitialiseFlashAccessProtection();	// Need flash protection in place before initialisation.
	/******************************************************/
	InitialiseFlash();
	InitialiseSatelliteOperations(flashProtectionSemaphore);
	InitialiseDiagnostics((uint16_t*)&buffer[0], 100);
	InitialiseDownlink();
	UplinkInit();
	
	Enable_global_interrupt();	/* Enable interrupts as the last thing we do. */
		
	//uiTraceStart();
	canOpenTaskHandle = xTaskCreateStatic(&CANOpenTask, "CAN Open", CANOPEN_STACK_SIZE, NULL, CAN_PRIORITY, (StackType_t * const)canOpenStack, (StaticTask_t * const)&canOpenControlStructure);
	downlinkTaskHandle = xTaskCreateStatic(&DownlinkThread, "Downlink", DOWNLINK_STACK_SIZE, NULL, DOWNLINK_PRIORITY, (StackType_t * const)downlinkStack, (StaticTask_t * const)&downlinkControlStructure);
	uplinkTaskHandle = xTaskCreateStatic(&UplinkThread, "Uplink", UPLINK_STACK_SIZE, NULL, UPLINK_PRIORITY, (StackType_t * const)uplinkStack, (StaticTask_t * const)&uplinkControlStructure);
	diagnosticsTaskHandle = xTaskCreateStatic(&DiagnosticsTask, "I2C Telemtery", DIAGNOSTICS_STACK_SIZE, NULL, DIAGNOSTICS_PRIORITY, (StackType_t * const)diagnosticsStack, (StaticTask_t * const)&diagnosticsControlStructure);
	opsTaskHandle = xTaskCreateStatic(&SatelliteOperationsThread, "Satellite Operations", OPS_STACK_SIZE, NULL, OPS_PRIORITY, (StackType_t * const)opsStack, (StaticTask_t * const)&opsControlStructure);
	payloadDataTaskHandle = xTaskCreateStatic(&PayloadDataThread, "Payload Data", PAYLOAD_DATA_STACK_SIZE, NULL, PAYLOAD_DATA_PRIORITY, (StackType_t * const)payloadDataStack, (StaticTask_t * const)&payloadDataControlStructure);
	Enable_global_interrupt();	/* Enable interrupts as the last thing we do. */		
	
	vTaskStartScheduler();		
}

/* Initialize clock buses with power clocks lib */
static void InitClock(void)
{
	pcl_freq_param_t myClockConfig;
	
	//clock definitions in "conf_clock.h"
	myClockConfig.main_clk_src = CLK_SRC;
	myClockConfig.cpu_f = CLK_CPU_F;
	myClockConfig.pba_f = CLK_PBA_F;
	myClockConfig.pbb_f = CLK_PBB_F;
	myClockConfig.pbc_f = CLK_PBC_F;
	myClockConfig.osc0_f = CLK_OSC0_F;
	myClockConfig.osc0_startup = CLK_OSC0_STARTUP;
	myClockConfig.dfll_f = CLK_DFLL_F; 
	myClockConfig.pextra_params = NULL;
	
	pcl_configure_clocks( &myClockConfig );
	
	return;
}

/* Statically allocate the timer task memory */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
	*ppxTimerTaskTCBBuffer	 = &timerControlStructure;
	*ppxTimerTaskStackBuffer = timerStack;
	*pulTimerTaskStackSize	 = configMINIMAL_STACK_SIZE;
}

/* Statically allocate the idle task memory */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
	*ppxIdleTaskTCBBuffer = &idleControlStructure;
	*ppxIdleTaskStackBuffer = idleStack;
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
