/*

	Project:       1Sheeld Bootloader

	File:          bootldr.h

	Compiler:      avr-gcc 3.4.2

	Author:        Shao ziyang (http://sourceforge.net/projects/avrub) with a little bit of modifications by Integreight, Inc. team.

	Date:          2014.5

*/

#ifndef _BOOTLDR_H_
#define _BOOTLDR_H_

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

//Define UART buffer's length
#define BUFFERSIZE			128

//Define system clock
#define F_CPU				7372800UL

//Define communication's baudrate
#define BAUDRATE			115200

//Boot section start address(byte)
#define BOOTSTARTADDRESS	0x1C00 * 2

//Basic timer interval in ms
#define TIMERINTERVAL		200

//Define the password waiting timout
//(Maximum password waiting time = TIMEOUTCOUNT * TIMERINTERVAL)
#define TIMEOUTCOUNT		1

//Define the data waiting timout
//(Maximum data waiting time = TIMEOUTCOUNTC * TIMERINTERVAL)
#define TIMEOUTCOUNTC		250

//Password character length
#define CONNECTCNT			8

//The password
unsigned char KEY[] = {0x64, 0x0E, 0x1C, 0x39, 0x14, 0x28, 0x57, 0xAA};

//Calculate baudrate register value
#define BAUDREG            ((unsigned int)((F_CPU * 10) / (16UL * BAUDRATE) - 5) / 10)

//Timer1 configuration: (prescaler: 1024, CTC mode: 4 and interval unit is millisecond)
#define timerInit()															\
{																			\
	OCR1A  = (unsigned int)(TIMERINTERVAL * (F_CPU  / (1024 * 1000.0f)));	\
	TCCR1A = 0;																\
	TCCR1B = (1 << WGM12)|(1 << CS12)|(1 << CS10);							\
}

//Configure SPI pins as output as a precaution
#define makeSPIPinsOutput()						\
{												\
	DDRB|=(1<<DDB5)|(1<<DDB6)|(1<<DDB7);		\
}

//Uart's UBBR value
#define UBRR_BOOT_VALUE 3

//Initialize Uart
#define initUART()												\
{                                                               \
	UBRR0H = (UBRR_BOOT_VALUE>>8);                              \
	UBRR0L = UBRR_BOOT_VALUE;                                   \
	UCSR0C = (1<<URSEL0) | (1 << UCSZ00)|(1 << UCSZ01);			\
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);						\
	UBRR1H = (UBRR_BOOT_VALUE>>8);								\
	UBRR1L = UBRR_BOOT_VALUE;                                   \
	UCSR1C = (1<<URSEL1) | (1 << UCSZ10)|(1 << UCSZ11);			\
	UCSR1B = (1 << RXEN1) | (1 << TXEN1);						\
}

//Used X-modem control commands
#define XMODEM_EOT         0x04
#define XMODEM_ACK         0x06
#define XMODEM_NAK         0x15
#define XMODEM_CAN         0x18
#define XMODEM_RWC         'C'
#define dataInCom()        (UCSR0A & (1 << RXC0))
#define readCom()          UDR0

#endif

//End of file: bootldr.h
