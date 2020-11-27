#ifndef _ADC_DRIVER_H
#define _ADC_DRIVER_H

#include <stdint.h>

/** 
Error codes for the ADC driver.
*/
typedef enum _ADC_ERROR
{
	NO_ERROR_ADC = 0,						/**< No errors detected. */
	COULD_NOT_INITIALISE_ADC,				/**< Initialisation failed. */
	ADC_NOT_INITIALISED,					/**< An attempt to access an ADC has been made without it being initialised. */
	INVALID_ADC_SPECIFIED,					/**< An invalid ADC has been specified for an operation. */
	INVALID_SENSOR_SELECTION_ADC,			/**< An invalid sensor name has been provided. */
	NO_BUFFER_PROVIDED_ADC,					/**< A buffer has not been provided to return data to. */
	COULD_NOT_SEND_PACKET_ADC,				/**< A packet transfer to an ADC failed. */
	INCORRECT_CHANNEL_REQUESTED_ADC,		/**< A channel has been requested that is not available on the specified ADC. */
	COULD_NOT_CONFIGURE_CHANNEL_ADC,		/**< Configuration of the ADC failed */
	COULD_NOT_READ_ADC						/**< A read request from an ADC has failed. */
}ADC_ERROR;

/* Public functions */
ADC_ERROR ADCInitialise(uint16_t * z_buffer, uint32_t z_size);
ADC_ERROR FetchDiagnosticsData(void);
void ADCUninitialise(void);

#endif