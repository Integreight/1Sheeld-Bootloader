/*

  Project:       AVR Universal BootLoader
  File:          bootcfg.h
                 user's configuration.
  Version:       3.5

  Compiler:      GCC 4.2.2 + AVR Studio 4.13sp2 b571

  Author:        Shaoziyang
                 Shaoziyang@gmail.com
                 http://avrubd.googlepages.com
                 
  Date:          2007.6
  Made by:       AVRUBD autocode, 09/01/2014 02:07:12 ã

  Modify:        Add your modify log here

  See readme.txt to get more information.

*/

#ifndef _BOOTCFG_H_
#define _BOOTCFG_H_        1

//define uart buffer's length
#define BUFFERSIZE         128

//system clock(Hz)
#ifndef F_CPU
#define F_CPU              7372800UL
#endif

//baudrate
#define BAUDRATE           57600

//Boot section start address(byte)
//define BootStart to 0 will disable this function
#define BootStart          0x1C00 * 2

//verify flash's data while write
//ChipCheck will only take effect while BootStart enable also
#define ChipCheck          1

//Bootloader launch  0:comport password  1:port level
#define LEVELMODE          0

#define LEVELPORT          D
#define LEVELPIN           PD7
//port level  1:High  0:Low
#define PINLEVEL           0

//max wait password time = TimeOutCnt * timeclk
//timeout count
#define TimeOutCnt         1

//basic timer interval(ms)
#define timeclk            200

//max wait data time = TimeOutCntC * timeclk
//send 'C' command count
#define TimeOutCntC        250

//password length
#define CONNECTCNT         8

//password
#if LEVELMODE == 0
unsigned char KEY[] = {0x64, 0x0E, 0x1C, 0x39, 0x14, 0x28, 0x57, 0xAA};  
#endif

//comport number: 0/1/2..
#define COMPORTNo          1

//enable watchdog
#define WDGEn              0

//enable RS485/RS422 mode
#define RS485              0
//RS485/RS422 send control port
#define RS485PORT          D
#define RS485TXEn          PD5 

//enable LED indication
#define LEDEn              1
//LED control port
#define LEDPORT            D
#define LEDPORTNo          PD6

//some kind of AVR need special delay after comport initialization
#define InitDelay          0 

//communication checksum method   0:CRC16  1:add up
#define CRCMODE            0

//Verbose mode: display more prompt message
#define VERBOSE            0

#endif

//End of file: bootcfg.h
