/*

	Project:       1Sheeld Bootloader

	File:          bootldr.c

	Compiler:      avr-gcc 3.4.2

	Author:        Shao Ziyang (http://sourceforge.net/projects/avrub) with a little bit of modifications by Integreight, Inc. team.

	Date:          2014.5

*/

#include "bootldr.h"

int main(void)
{
	//Define some variables
	unsigned char cnt;
	unsigned char packNO;
	unsigned char crch, crcl;
	unsigned char li;
	
	//As a precaution, make SPI pins as output
	makeSPIPinsOutput();
	
	//Disable global interrupts
	cli();
	
	//Disable the watchdog timer
	disableWatchdogTimer();
	
	//Initialize Timer1 in CTC mode
	initTimer1();
	
	//Initialize UART1 with baudrate 115200
	initUART1();

	//Initialize the cnt with the password timeout count
	cnt = TIMEOUTCOUNT;
	cl = 0;
	
	//Send a starting NAK byte to notify host that we entered the bootloader section
	sendByte(XMODEM_NAK);
	
	//The 8 byte password receiving loop
	while(1)
	{
		//If Timer1 overflow, then 200ms have passed
		if(TIFR & (1<<OCF1A))   
		{
			TIFR |= (1 << OCF1A);
			
			//If we have already read 8 bytes
			if(cl == CONNECTCNT)      
			break;
			
			//Decrement the number of retries
			cnt--;
			
			//If we finished the number of retries, quit to the firmware
			if(cnt == 0)
			{
				quitToFirmwareApplication();
			}
		}

		if(isUartDataAvailable())
		{
			//Compare the key byte with its corresponding received byte
			if(readByte() == KEY[cl])
				cl++;
			else
				cl = 0;
		}
	}
	
	//Initialize the cnt with the password timeout count
	cnt = TIMEOUTCOUNTC;
	
	//Every 200ms send a "C" and wait for the first byte
	while(1)
	{
		//If Timer1 overflow, then 200ms have passed
		if(TIFR & (1 << OCF1A))  //T1 overflow
		{
			TIFR |= (1 << OCF1A);
			
			//send "C"
			sendByte(XMODEM_RWC);
			
			//Decrement the number of retries
			cnt--;
			
			//If we finished the number of retries, quit to the firmware
			if(cnt == 0)
			{
				quitToFirmwareApplication();
			}
		}
		
		if(isUartDataAvailable())
		{
			break;
		}
	}

	//Reset Timer1
	TCCR1B = 0;

	//Initialize some needed variables
	packNO    = 0;
	bufptr    = 0;
	cnt       = 0;
	flashAddress = 0;
	
	//Begin to receive data
	
	//Check if we received the end of transmission (The first time this character will be SOH)
	while(waitForTheNextByteAndReadIt() != XMODEM_EOT)
	{
		//The next expected packet number
		packNO++;
		
		//Read the packet number
		receivedPacketNumber    =  waitForTheNextByteAndReadIt();
		
		//Read the packet number complement
		packetNumberComplement	= ~waitForTheNextByteAndReadIt();
		
		//Receive a full data frame
		for(li = 0; li < BUFFERSIZE; li++)
		{
			buf[bufptr] = waitForTheNextByteAndReadIt();
			bufptr++;
		}
		
		//Read the CRC bytes
		crch = waitForTheNextByteAndReadIt();
		crcl = waitForTheNextByteAndReadIt();
		
		//Check if the received packet number and its complement matches the expected values
		if ((packNO == receivedPacketNumber) && (packNO == packetNumberComplement))
		{
			//Calculate the CRC checksum
			calculateCRCChecksum(&buf[bufptr - BUFFERSIZE]);
			
			//Check if it is the same as the received ones
			if((crch == ch) && (crcl == cl))
			{
				//Avoid writing to the boot section
				if(flashAddress < BOOTSTARTADDRESS)
				{
					//If flash page is full
					if(bufptr >= SPM_PAGESIZE)
					{
						//Write the flash page
						writeOneFlashPage(buf);
						
						//Modify the flash page address
						flashAddress += SPM_PAGESIZE;
						
						//Reset the received data buffer pointer
						bufptr = 0;
					}
				}
				
				//Ignore flash write when flash address exceeds BootStart
				else                                  
				{
					//Reset the received data buffer pointer
					bufptr = 0;
				}
				
				//Read flash and compare it with the buffer's content
				if(flashAddress < BOOTSTARTADDRESS)
				{
					//Enable the application section
					boot_rww_enable();
					
					//Clear the error flag
					cl = 1;
					
					for(pagptr = 0; pagptr < BUFFERSIZE; pagptr++)
					{
						if(pgm_read_byte(flashAddress - BUFFERSIZE + pagptr) != buf[pagptr])
						{
							//Set the error flag and break from the loop
							cl = 0;                         
							break;
						}
					}
					
					//If CRC bytes are equal, send ACK
					if(cl)
					{
						sendByte(XMODEM_ACK);
						cnt = 0;
					}
					
					//CRC checksum error, tell the host to resend
					else
					{
						//Return the packet number to the expected value
						packNO--;
						
						//Reset the received data buffer pointer
						bufptr = 0;
						
						//Send NAK byte
						sendByte(XMODEM_NAK);
						
						//Increase the error count
						cnt++;
						
						//Modify the flash page address
						flashAddress -= BUFFERSIZE;
					}
				}
				else                                  //don't need verify, send ACK directly
				{
					sendByte(XMODEM_ACK);
					cnt = 0;
				}
			}
			
			//The CRC values are not the expected ones, tell the host to resend
			else
			{
				//Return the packet number to the expected value
				packNO--;
				
				//Reset the received data buffer pointer
				bufptr = 0;
				
				//Send NAK byte
				sendByte(XMODEM_NAK);
				
				//Increase the error count
				cnt++;
			}
		}
		
		//The received packet number and its complement don't match the expected values
		else
		{
			//Return the packet number to the expected value
			packNO--;
			
			//Reset the received data buffer pointer
			bufptr = 0;
			
			//Send NAK byte
			sendByte(XMODEM_NAK);
			
			//Increase the error count
			cnt++;
		}
		
		//If there is too many errors, abort
		if(cnt > 3)
		{
			break;
		}
		
	}
	
	//If there is no errors, send ACK
	if(cnt == 0)
	{
		sendByte(XMODEM_ACK);
	}
	else
	{
		sendByte(XMODEM_CAN);
	}
	
	//Go to the firmware all times
	quitToFirmwareApplication();
	return 0;
}

//End of file: bootldr.c
