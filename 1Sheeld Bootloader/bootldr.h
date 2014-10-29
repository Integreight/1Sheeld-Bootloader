/*

  Project:       1Sheeld Bootloader
  
  File:          bootldr.h
                
  Compiler:      avr-gcc 3.4.2

  Author:        Integreight based on AVR Universal Bootloader by Shao ziyang: http://sourceforge.net/projects/avrub
                 
  Date:          2014.5

*/

#ifndef _BOOTLDR_H_
#define _BOOTLDR_H_        1

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>


//define uart buffer's length
#define BUFFERSIZE         128

//system clock(Hz)
#ifndef F_CPU
#define F_CPU              7372800UL
#endif

//baudrate
#define BAUDRATE           115200

//Boot section start address(byte)
//define BootStart to 0 will disable this function
#define BOOTSTARTADDRESS          0x1C00 * 2

//max wait password time = TimeOutCnt * timeclk
//timeout count
#define TIMEOUTCOUNT         1

//basic timer interval(ms)
#define TIMERINTERVAL        200

//max wait data time = TimeOutCntC * timeclk
//send 'C' command count
#define TIMEOUTCOUNTC        250

//password length
#define CONNECTCNT         8

//password
//#if LEVELMODE == 0
unsigned char KEY[] = {0x64, 0x0E, 0x1C, 0x39, 0x14, 0x28, 0x57, 0xAA};
//#endif

//comport number: 0/1/2..
#define COMPORTNO          1

///////////////////////////////////////////////////////////////
//Don't modify code below, unless your really konw what to do//
///////////////////////////////////////////////////////////////

//certain version compiler missing this in head files
//#ifndef SPM_PAGESIZE
//#error "Not define SPM_PAGESIZE, please define below or update your WinAVR"
////#define SPM_PAGESIZE       XXX
//#endif

////certain version compiler missing this in head files
//#ifndef FLASHEND
//#error "Not define FLASHEND, please define below or update your WinAVR"
////#define FLASHEND           XXX
//#endif

//two buffer size must be multiple or submultiple relation
//#if BUFFERSIZE >= SPM_PAGESIZE
//#if (BUFFERSIZE / SPM_PAGESIZE * SPM_PAGESIZE) != BUFFERSIZE
//#error "Result of (BUFFERSIZE / SPM_PAGESIZE) is not a Integer!"
//#error "Please check and set 'BUFFERSIZE/SPM_PAGESIZE' Macro again!"
//#endif
//#else
//#if (SPM_PAGESIZE /BUFFERSIZE * BUFFERSIZE) != SPM_PAGESIZE
//#error "Result of (BUFFERSIZE / SPM_PAGESIZE) is not a Integer!"
//#error "Please check and set 'BUFFERSIZE/SPM_PAGESIZE' Macro again!"
//#endif
//#endif

#ifndef BAUD
#define BAUD BAUDRATE
#endif

//calculate baudrate register
#define BAUDREG            ((unsigned int)((F_CPU * 10) / (16UL * BAUDRATE) - 5) / 10)

//check baudrate register error
//mocro below maybe not same in different C compiler
#define freqTemp           (16UL * BAUDRATE * (((F_CPU * 10) / (16 * BAUDRATE) + 5)/ 10))
//#if ((FreqTemp * 50) > (51 * F_CPU)) || ((FreqTemp * 50) < (49 * F_CPU))
//#error "BaudRate error > 2% ! Please check BaudRate and F_CPU value."
//#endif

//#ifndef F_CPU
//#define F_CPU 7372800UL
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


//timer1: prescale 1024, CTC mode 4, interval unit is millisecond
#define timerInit()															\
{																			\
	OCR1A  = (unsigned int)(TIMERINTERVAL * (F_CPU  / (1024 * 1000.0f)));	\
    TCCR1A = 0;																\
    TCCR1B = (1 << WGM12)|(1 << CS12)|(1 << CS10);							\
}

// make spi pins output
#define makeSPIPinsOutput()						\
{												\
	DDRB|=(1<<DDB5)|(1<<DDB6)|(1<<DDB7);		\
}

//timer1 overflow register
#ifdef TIFR
#define TIFRREG            TIFR
#else
#define TIFRREG            TIFR1
#endif

#define UBRR_BOOT_VALUE 3 

//Initialize USART
#define initUART()												\
{                                                               \
	UBRR0H = (UBRR_BOOT_VALUE>>8);                              \
	UBRR0L = UBRR_BOOT_VALUE;                                   \
	UCSR0C = (1<<URSEL0) | (1 << UCSZ00)|(1 << UCSZ01);			\
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);						\
	UCSR0A = 2;													\
	UBRR1H = (UBRR_BOOT_VALUE>>8);								\
	UBRR1L = UBRR_BOOT_VALUE;                                   \
	UCSR1C = (1<<URSEL1) | (1 << UCSZ10)|(1 << UCSZ11);			\
	UCSR1B = (1 << RXEN1) | (1 << TXEN1);						\
}

//Xmodem control commands
#define XMODEM_NUL         0x00
#define XMODEM_SOH         0x01
#define XMODEM_STX         0x02
#define XMODEM_EOT         0x04
#define XMODEM_ACK         0x06
#define XMODEM_NAK         0x15
#define XMODEM_CAN         0x18
#define XMODEM_EOF         0x1A
#define XMODEM_RWC         'C'
#define dataInCom()        (UCSRAREG(COMPORTNO) & (1 << RXCBIT(COMPORTNO)))
#define readCom()          UDRREG(COMPORTNO)

#endif

//End of file: bootldr.h
