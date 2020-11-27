
#ifndef FLASHC_H_
#define FLASHC_H_

void ClearPage(void);
void ClearPageBuffer(void);
void WriteToFlash(void);

// Not really the ideal place for these but limited header files to choose from.
 #define __attribute__(x)
#define __interrupt__	

#endif