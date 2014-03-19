#ifndef _VSARDUINO_H_
#define _VSARDUINO_H_
//Board = Arduino Mega 2560 or Mega ADK
#define __AVR_ATmega2560__
#define ARDUINO 101
#define ARDUINO_MAIN
#define __AVR__
#define __avr__
#define F_CPU 16000000L
#define __cplusplus
#define __inline__
#define __asm__(x)
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __asm__ 
#define __volatile__

#define __builtin_va_list
#define __builtin_va_start
#define __builtin_va_end
#define __DOXYGEN__
#define __attribute__(x)
#define NOINLINE __attribute__((noinline))
#define prog_void
#define PGM_VOID_P int
            
typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {;}

void setup(void);
void loop(void);
void doWifiStuff();
void checkBuzzerStatus();
void doRFIDstuff();
char* getTagNumber();
uint8_t AsciiCharToNum(byte data);

#include "D:\_Development\arduino-1.0.5-r2\hardware\arduino\cores\arduino\arduino.h"
#include "D:\_Development\arduino-1.0.5-r2\hardware\arduino\variants\mega\pins_arduino.h" 
#include "D:\My Documents\Arduino\CC3000\CC3000_ITU\CC3000_ITU.ino"
#endif
