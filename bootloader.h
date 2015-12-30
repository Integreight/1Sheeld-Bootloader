/*

	Project:       1Sheeld Bootloader

	File:          bootldr.h

	Compiler:      avr-gcc 3.4.2

	Author:        Shao Ziyang (http://sourceforge.net/projects/avrub) with a little bit of modifications by Integreight, Inc. team.

	Date:          2014.5

*/

#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_

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

//The firmware application start address
#define PROG_START         0x0000

//Used X-modem control commands
#define XMODEM_EOT         0x04
#define XMODEM_ACK         0x06
#define XMODEM_NAK         0x15
#define XMODEM_CAN         0x18
#define XMODEM_RWC         'C'
#define isUartDataAvailable()        (UCSR1A & (1 << RXC1))
#define readByte()          UDR1

//Timer1 configuration: (prescaler: 1024, CTC mode: 4 and interval unit is millisecond)
#define initTimer1()															\
{																			\
	OCR1A  = (unsigned int)(TIMERINTERVAL * (F_CPU  / (1024 * 1000.0f)));	\
	TCCR1B = (1 << WGM12)|(1 << CS12)|(1 << CS10);							\
}

//Configure SPI pins as output as a precaution
#define makeSPIPinsOutput()						\
{												\
	DDRB|=(1<<DDB5)|(1<<DDB6)|(1<<DDB7);		\
}

//Disable watchdog timer
#define disableWatchdogTimer()					\
{												\
	MCUCSR = 0;									\
	wdt_disable();								\
}

//Define UART1 UBBR value
#define UBRR_BOOT_VALUE 3

//Initialize UART1
#define initUART1()												\
{                                                               \
	UBRR1L = UBRR_BOOT_VALUE;                                   \
	UCSR1C = (1<<URSEL1) | (1 << UCSZ10)|(1 << UCSZ11);			\
	UCSR1B = (1 << RXEN1) | (1 << TXEN1);						\
}

//Define a receive buffer and some variables
unsigned char buf[BUFFERSIZE];
unsigned char bufptr, pagptr;
unsigned char ch, cl, receivedPacketNumber, packetNumberComplement;
unsigned int flashAddress;

//Write one flash page
void writeOneFlashPage(unsigned char *buf)
{
	//Erase one flash page
	boot_page_erase(flashAddress);
	//Wait till the flash page is erased
	boot_spm_busy_wait();
	//Fill data to the flash buffer
	for(pagptr = 0; pagptr < SPM_PAGESIZE; pagptr += 2)
	{
		boot_page_fill(pagptr, buf[pagptr] + (buf[pagptr + 1] << 8));
	}
	//Write buffer to one flash page
	boot_page_write(flashAddress);
	//Wait till the flash page is written
	boot_spm_busy_wait();
}

//Quit the bootloader and jump to the firmware application
void quitToFirmwareApplication()
{
	//Reset Timer1 register
	TCCR1B = 0;
	//Enable the application section
	boot_rww_enable();
	//Jump to the firmware
	(*((void(*)(void))PROG_START))();
}

//Send a byte to the UART1
void sendByte(unsigned char data)
{
	UDR1 = data;
	//Wait for the sending to finish
	while(!(UCSR1A & (1<<TXC1)));
	UCSR1A |= (1 << TXC1);
}

//Wait to receive a byte from UART1
unsigned char waitForTheNextByteAndReadIt()
{
	while(!isUartDataAvailable());
	return readByte();
}

//Calculate CRC checksum
void calculateCRCChecksum(unsigned char *buf)
{
	unsigned char j;
	unsigned char i;
	unsigned int t;
	unsigned int crc;
	crc = 0;
	for(j = BUFFERSIZE; j > 0; j--)
	{
		//CRC1021 checksum
		crc = (crc ^ (((unsigned int) *buf) << 8));
		for(i = 8; i > 0; i--)
		{
			t = crc << 1;
			if(crc & 0x8000)
			t = t ^ 0x1021;
			crc = t;
		}
		buf++;
	}
	ch = crc / 256;
	cl = crc % 256;
}

#endif

//End of file: bootloader.h
