/*

	Project:       1Sheeld Bootloader

	File:          bootldr.c

	Compiler:      avr-gcc 3.4.2

	Author:        Shao ziyang (http://sourceforge.net/projects/avrub) with a little bit of modifications by Integreight, Inc. team.

	Date:          2014.5

*/

#include "bootldr.h"

//user's application start address
#define PROG_START         0x0000

//define receive buffer
unsigned char buf[BUFFERSIZE];
unsigned char bufptr, pagptr;
unsigned char ch, cl, RecivedPacketNo, PacketNoComplement;
unsigned int FlashAddr;

//write one Flash page
void writeOneFlashPage(unsigned char *buf)
{
	boot_page_erase(FlashAddr);                  //erase one Flash page
	boot_spm_busy_wait();
	for(pagptr = 0; pagptr < SPM_PAGESIZE; pagptr += 2) //fill data to Flash buffer
	{
		boot_page_fill(pagptr, buf[pagptr] + (buf[pagptr + 1] << 8));
	}
	boot_page_write(FlashAddr);                  //write buffer to one Flash page
	boot_spm_busy_wait();                        //wait Flash page write finish
}

//jump to user's application
void quitToUserApplication()
{
	TCCR1B = 0;
	boot_rww_enable();                           //enable application section
	(*((void(*)(void))PROG_START))();            //jump
}

//send a byte to comport
void sendByte(unsigned char dat)
{
	UDR0 = dat;
	//wait send finish
	while(!(UCSR0A & (1<<TXC0)));
	UCSR0A |= (1 << TXC0);
}

//wait receive a data from comport
unsigned char readUARTData()
{
	while(!dataInCom());
	return readCom();
}

//calculate CRC checksum
void calculateCRC(unsigned char *buf)
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


//Main routine
int main(void)
{
	unsigned char cnt;
	unsigned char packNO;
	unsigned char crch, crcl;
	unsigned char li;
	makeSPIPinsOutput();
	//disable global interrupts
	cli();
	//disable watchdog
	MCUCSR = 0;
	wdt_disable();
	//initialize timer1, CTC mode
	timerInit();
	//initialize commport with special config value
	initUART();
	//comport launch boot
	cnt = TIMEOUTCOUNT;
	cl = 0;
	//Send NAK byte
	sendByte(XMODEM_NAK);
	while(1)
	{
		if(TIFR & (1<<OCF1A))    //T1 overflow
		{
			TIFR |= (1 << OCF1A);

			if(cl == CONNECTCNT)      //determine Connect Key
			break;

			cnt--;
			if(cnt == 0)              //connect timeout
			{
				quitToUserApplication();                 //quit bootloader
			}
		}

		if(dataInCom())             //receive connect key
		{
			if(readCom() == KEY[cl])  //compare ConnectKey
			cl++;
			else
			cl = 0;
		}
	}
	//every interval send a "C",waiting XMODEM control command <soh>
	cnt = TIMEOUTCOUNTC;
	while(1)
	{
		if(TIFR & (1 << OCF1A))  //T1 overflow
		{
			TIFR |= (1 << OCF1A);
			sendByte(XMODEM_RWC) ;    //send "C"
			cnt--;
			if(cnt == 0)              //timeout
			{
				quitToUserApplication();                 //quit bootloader
			}
		}
		if(dataInCom())
		{
			//if(ReadCom() == XMODEM_SOH)  //XMODEM command <soh>
			break;
		}
	}

	TCCR1B = 0;                   //close timer1

	//begin to receive data
	packNO    = 0;
	bufptr    = 0;
	cnt       = 0;
	FlashAddr = 0;
	while(readUARTData() != XMODEM_EOT)
	{
		packNO++;
		RecivedPacketNo    =  readUARTData();           //get package number
		PacketNoComplement = ~readUARTData();
		for(li = 0; li < BUFFERSIZE; li++)      //receive a full data frame
		{
			buf[bufptr] = readUARTData();
			bufptr++;
		}
		crch = readUARTData();                       //get CRC
		crcl = readUARTData();
		if ((packNO == RecivedPacketNo) && (packNO == PacketNoComplement))
		{
			calculateCRC(&buf[bufptr - BUFFERSIZE]);       //calculate checksum
			if((crch == ch) && (crcl == cl))
			{
				if(FlashAddr < BOOTSTARTADDRESS)             //avoid write to boot section
				{
					if(bufptr >= SPM_PAGESIZE)          //Flash page full, write flash page;otherwise receive next frame
					{                                   //receive multi frames, write one page
						writeOneFlashPage(buf);              //write data to Flash
						FlashAddr += SPM_PAGESIZE;        //modify Flash page address
						bufptr = 0;
					}
				}
				else                                  //ignore flash write when Flash address exceed BootStart
				{
					bufptr = 0;                         //reset receive pointer
				}
				//read flash, and compare with buffer's content
				if(FlashAddr < BOOTSTARTADDRESS)
				{
					boot_rww_enable();                  //enable application section
					cl = 1;                             //clear error flag
					for(pagptr = 0; pagptr < BUFFERSIZE; pagptr++)
					{
						if(pgm_read_byte(FlashAddr - BUFFERSIZE + pagptr) != buf[pagptr])
						{
							cl = 0;                         //set error flag
							break;
						}
					}
					if(cl)                              //checksum equal, send ACK
					{
						sendByte(XMODEM_ACK);
						cnt = 0;
					}
					else
					{
						sendByte(XMODEM_NAK);             //checksum error, ask resend
						cnt++;                            //increase error counter
						packNO--;
						bufptr = 0;
						FlashAddr -= BUFFERSIZE;             //modify Flash page address
					}
				}
				else                                  //don't need verify, send ACK directly
				{
					sendByte(XMODEM_ACK);
					cnt = 0;
				}
			}
			else                                    //CRC
			{
				packNO--;
				bufptr = 0;
				sendByte(XMODEM_NAK);                 //require resend
				cnt++;
			}
		}
		else //PackNo
		{
			packNO--;
			bufptr = 0;                               //reinitialize the pointer
			cnt++;
			sendByte(XMODEM_NAK);                    //require resend
		}

		if(cnt > 3)                               //too many error, abort update
		{
			break;
		}
		
	}
	if(cnt == 0)
	{
		sendByte(XMODEM_ACK);
	}
	else
	{
		sendByte(XMODEM_CAN);
	}
	quitToUserApplication();                                     //quit bootloader
	return 0;
}

//End of file: bootldr.c
