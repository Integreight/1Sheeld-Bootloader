/*

  Project:       1Sheeld Bootloader
  
  File:          bootldr.h
                
  Compiler:      avr-gcc 3.4.2

  Author:        Integreight based on AVR Universal Bootloader by Shaoziyang: http://sourceforge.net/projects/avrub
                 
  Date:          2014.5

*/

#ifndef _BOOTLDR_H_
#define _BOOTLDR_H_        1

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

///////////////////////////////////////////////////////////////
//Don't modify code below, unless your really konw what to do//
///////////////////////////////////////////////////////////////

//certain version compiler missing this in head files
#ifndef SPM_PAGESIZE
#error "Not define SPM_PAGESIZE, please define below or update your WinAVR"
//#define SPM_PAGESIZE       XXX
#endif

//certain version compiler missing this in head files
#ifndef FLASHEND
#error "Not define FLASHEND, please define below or update your WinAVR"
//#define FLASHEND           XXX
#endif

//two buffer size must be multiple or submultiple relation
#if BUFFERSIZE >= SPM_PAGESIZE
#if (BUFFERSIZE / SPM_PAGESIZE * SPM_PAGESIZE) != BUFFERSIZE
#error "Result of (BUFFERSIZE / SPM_PAGESIZE) is not a Integer!"
#error "Please check and set 'BUFFERSIZE/SPM_PAGESIZE' Macro again!"
#endif
#else
#if (SPM_PAGESIZE /BUFFERSIZE * BUFFERSIZE) != SPM_PAGESIZE
#error "Result of (BUFFERSIZE / SPM_PAGESIZE) is not a Integer!"
#error "Please check and set 'BUFFERSIZE/SPM_PAGESIZE' Macro again!"
#endif
#endif

//calculate baudrate register
#define BAUDREG            ((unsigned int)((F_CPU * 10) / (16UL * BAUDRATE) - 5) / 10)

//check baudrate register error
//mocro below maybe not same in different C compiler
#define freqTemp           (16UL * BAUDRATE * (((F_CPU * 10) / (16 * BAUDRATE) + 5)/ 10))
//#if ((FreqTemp * 50) > (51 * F_CPU)) || ((FreqTemp * 50) < (49 * F_CPU))
//#error "BaudRate error > 2% ! Please check BaudRate and F_CPU value."
//#endif

//internal use macro
#define CONCAT(a, b)       a ## b
#define CONCAT3(a, b, c)   a ## b ## c

//register of PORT and bit define
#define PORTREG(No)        CONCAT(PORT, No)
#define PINREG(No)         CONCAT(PIN, No)
#define UDRREG(No)         CONCAT(UDR, No)
#define DDRREG(No)         CONCAT(DDR, No)
#define TXCBIT(No)         CONCAT(TXC, No)
#define RXCBIT(No)         CONCAT(RXC, No)
#define RXENBIT(No)        CONCAT(RXEN, No)
#define TXENBIT(No)        CONCAT(TXEN, No)
#define URSELBIT(No)       CONCAT(URSEL, No)

//comport register
#define UBRRHREG(No)       CONCAT3(UBRR, No, H)
#define UBRRLREG(No)       CONCAT3(UBRR, No, L)
#define UCSRAREG(No)       CONCAT3(UCSR, No, A)
#define UCSRBREG(No)       CONCAT3(UCSR, No, B)
#define UCSRCREG(No)       CONCAT3(UCSR, No, C)
#define UCSZBIT(No1, No2)  CONCAT3(UCSZ, No1, No2)

//some kind of AVR need URSEL define when comport initialize
#if defined(URSEL) || defined(URSEL0)
#define USEURSEL           (1 << URSELBIT(COMPORTNo))
#else
#define USEURSEL           0
#endif

//define UART0 register
#if !defined(UDR0)
#define UBRR0H             UBRRH
#define UBRR0L             UBRRL
#define UCSR0A             UCSRA
#define UCSR0B             UCSRB
#define UCSR0C             UCSRC
#define UDR0               UDR
#define TXC0               TXC
#define RXC0               RXC
#define RXEN0              RXEN
#define TXEN0              TXEN
#define UCSZ01             UCSZ1
#define UCSZ00             UCSZ0
#define URSEL0             URSEL
#endif

//initialize comport
#define comInit()                                                         \
        {                                                                 \
                           UCSRAREG(COMPORTNo) = 0;                       \
                           UCSRBREG(COMPORTNo) = (1 << RXENBIT(COMPORTNo))|(1 << TXENBIT(COMPORTNo)); \
                           UCSRCREG(COMPORTNo) = USEURSEL|(1 << UCSZBIT(COMPORTNo, 1))|(1 << UCSZBIT(COMPORTNo, 0));\
                           UBRRHREG(COMPORTNo) = BAUDREG/256;             \
                           UBRRLREG(COMPORTNo) = BAUDREG%256;             \
        }
		
//toggle LED output
#define ledAlt()           PORTREG(LEDPORT) ^= (1 << LEDPORTNo)

//timer1: prescale 1024, CTC mode 4, interval unit is millisecond
#define timerInit()                                                       \
        {                                                                 \
                           OCR1A  = (unsigned int)(timeclk * (F_CPU  / (1024 * 1000.0f)));\
                           TCCR1A = 0;                                    \
                           TCCR1B = (1 << WGM12)|(1 << CS12)|(1 << CS10); \
        }

//timer1 overflow register
#ifdef TIFR
#define TIFRREG            TIFR
#else
#define TIFRREG            TIFR1
#endif

//Xmodem control commands
#define XMODEM_NUL         0x00
#define XMODEM_SOH         0x01
#define XMODEM_STX         0x02
#define XMODEM_EOT         0x04
#define XMODEM_ACK         0x06
#define XMODEM_NAK         0x15
#define XMODEM_CAN         0x18
#define XMODEM_EOF         0x1A
#if CRCMODE
#define XMODEM_RWC         XMODEM_NAK
#else
#define XMODEM_RWC         'C'
#endif


#if RS485
#define rs485Enable()      PORTREG(RS485PORT) |= (1 << RS485TXEn)
#define rs485Disable()     PORTREG(RS485PORT) &= ~(1 << RS485TXEn)
#endif

#define dataInCom()        (UCSRAREG(COMPORTNo) & (1 << RXCBIT(COMPORTNo)))
#define readCom()          UDRREG(COMPORTNo)

#endif

//End of file: bootldr.h
