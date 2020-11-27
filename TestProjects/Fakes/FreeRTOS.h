#ifndef INC_FREERTOS_H
#define INC_FREERTOS_H
#include <stdint.h>


#define portTICK_RATE_MS 1

/* This might need to be replaced with a real function when we come to doing proper testing but for now we can leave as is. */
void vTaskDelay(uint32_t xTicksToDelay);

#endif