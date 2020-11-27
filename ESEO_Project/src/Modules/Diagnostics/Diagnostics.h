/**
@file 	Diagnostics.h
@author Pete Bartram
@brief 
*/

#ifndef _DIAGNOSTICS_H
#define _DIAGNOSTICS_H

#include <stdint.h>
#include "Sensors.h"

/**
Error list for the Diagnostics system.
*/
typedef enum _DIAGNOSTICS_ERROR
{
	NO_ERROR_DIAG = 0,				/**< No errors detected. */
	INITIALISATION_FAILED_DIAG,		/**< Initialisation failed - check input parameters. */
	NOT_INITIALISED_DIAG,			/**< Data has been requested before initialisation was performed. */
	BUFFER_OVERFLOW_DIAG,			/**< A request has been made for more data than the buffer can handle, check the size of the SENSOR_NAME list */
	FETCH_DATA_FAILED_DIAG
}DIAGNOSTICS_ERROR;

/* Public function prototypes */
DIAGNOSTICS_ERROR InitialiseDiagnostics(uint16_t * z_diagnosticData, uint32_t z_size);
DIAGNOSTICS_ERROR FetchNewDiagnosticData(void);
void UninitialiseDiagnostics(void);
void DiagnosticsTask(void);

#endif