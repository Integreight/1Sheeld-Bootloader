/*

  Project:       1Sheeld Bootloader
  
  File:          bootldr.c
                
  Compiler:      avr-gcc 3.4.2

  Author:        Integreight based on AVR Universal Bootloader by Shaoziyang: http://sourceforge.net/projects/avrub
                 
  Date:          2014.5

*/
#ifndef F_CPU
#define F_CPU 7372800UL
#endif

#ifndef BAUD
# define BAUD 115200
#endif

#define UBRR_BOOT_VALUE 3 

#include "bootcfg.h"
#include "bootldr.h"
#include <util/setbaud.h>

//user's application start address
#define PROG_START         0x0000

//receive buffer' size will not smaller than SPM_PAGESIZE
#if (BUFFERSIZE < SPM_PAGESIZE)
#define BUFSIZE SPM_PAGESIZE
#else
#define BUFSIZE BUFFERSIZE
#endif

//define receive buffer
unsigned char buf[BUFSIZE];

const char msg1[] = "1";//"waiting for password";
const char msg3[] = "3";//"waiting for data";

#if (BUFSIZE > 255)
unsigned int bufptr, pagptr;
#else
unsigned char bufptr, pagptr;
#endif

unsigned char ch, cl, RecivedPacketNo, PacketNoComplement;

//Flash address
#if (FLASHEND > 0xFFFFUL)
unsigned long int FlashAddr;
#else
unsigned int FlashAddr;
#endif

//include decrypt subroutine file
#if Decrypt

//PC1 decrypt algorithm subroutine
#if (Algorithm == 0)||(Algorithm == 1)

#include "pc1crypt.c"

#else

#error "Unknow encrypt algorithm!"

#endif

#endif  //Decrypt


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
#if Decrypt
    DestroyKey();                              //delete decrypt key
#endif
  TCCR1B = 0;
  boot_rww_enable();                           //enable application section
  (*((void(*)(void))PROG_START))();            //jump
}

//send a byte to comport
void sendByte(unsigned char dat)
{
#if RS485
  rs485Enable();
#endif

  UDRREG(COMPORTNo) = dat;
  //wait send finish
  while(!(UCSRAREG(COMPORTNo) & (1<<TXCBIT(COMPORTNo))));
  UCSRAREG(COMPORTNo) |= (1 << TXCBIT(COMPORTNo));

#if RS485
  rs485Disable();
#endif
}

//Initialize USART 
void initUART(void)                                                                    
{                                                                              
	UBRR0H = (UBRR_BOOT_VALUE>>8);                                            
	UBRR0L = UBRR_BOOT_VALUE;                                                 
	UCSR0C = (1<<URSEL0) | (1 << UCSZ00)|(1 << UCSZ01); 
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);               
	UCSR0A = 2;
	UBRR1H = (UBRR_BOOT_VALUE>>8);                                            
	UBRR1L = UBRR_BOOT_VALUE;                                                 
	UCSR1C = (1<<URSEL1) | (1 << UCSZ10)|(1 << UCSZ11); 
	UCSR1B = (1 << RXEN1) | (1 << TXEN1);               
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
#if (BUFSIZE > 255)
  unsigned int j;
#else
  unsigned char j;
#endif

#if (CRCMODE == 0)
  unsigned char i;
  unsigned int t;
#endif
  unsigned int crc;

  crc = 0;
  for(j = BUFFERSIZE; j > 0; j--)
  {
#if (CRCMODE == 0)
    //CRC1021 checksum
    crc = (crc ^ (((unsigned int) *buf) << 8));
    for(i = 8; i > 0; i--)
    {
      t = crc << 1;
      if(crc & 0x8000)
        t = t ^ 0x1021;
      crc = t;
    }
#elif (CRCMODE == 1)
    //word add up checksum
    crc += (unsigned int)(*buf);
#else
#error "Unknow CRCMODE!"
#endif
    buf++;
  }
  ch = crc / 256;
  cl = crc % 256;
}

// make spi pins output
void makeSPIPinsOutput()
{
	DDRB|=(1<<DDB5)|(1<<DDB6)|(1<<DDB7);
}
//Main routine
int main(void)
{
  unsigned char cnt;
  unsigned char packNO;
  makeSPIPinsOutput();
  #if   (CRCMODE == 0)
	    unsigned char crch, crcl; 
  #elif (CRCMODE == 1)
	     unsigned char checksum;
  #endif
#if (InitDelay > 0)
#if (InitDelay > 255)
  unsigned int di;
#else
  unsigned char di;
#endif
#endif

#if (BUFFERSIZE > 255)
  unsigned int li;
#else
  unsigned char li;
#endif

  //disable global interrupts
  cli();

#if WDGEn
  //if enable watchdog, setup timeout
  wdt_enable(WDTO_1S);
#else
  //disable watchdog
  MCUCSR = 0;
  wdt_disable();
#endif

  //initialize timer1, CTC mode
  timerInit();

#if RS485
  //initialize RS485 port
  DDRREG(RS485PORT) |= (1 << RS485TXEn);
  rs485Disable();
#endif

#if LED_En
  //set LED control port to output
  DDRREG(LEDPORT) = (1 << LEDPORTNo);
#endif

  //initialize commport with special config value
  initUART();  
#if (InitDelay > 0)
  //some kind of avr mcu need special delay after comport initialization
  for(di = InitDelay; di > 0; di--)
    __asm__ __volatile__ ("nop": : );
#endif

#if LEVELMODE
  //according port level to enter bootloader
  //set port to input
  DDRREG(LEVELPORT) &= ~(1 << LEVELPIN);
#if PINLEVEL
  if(PINREG(LEVELPORT) & (1 << LEVELPIN))
#else
  if(!(PINREG(LEVELPORT) & (1 << LEVELPIN)))
#endif
  {}
  else
  {
    quitToUserApplication();
  }

#else
  //comport launch boot
  cnt = TimeOutCnt;
  cl = 0;
  //Send NAK byte 
  sendByte(XMODEM_NAK);
  while(1)
  {
	  
#if WDG_En
    //clear watchdog
    wdt_reset();
#endif

    if(TIFRREG & (1<<OCF1A))    //T1 overflow
    {
      TIFRREG |= (1 << OCF1A);

      if(cl == CONNECTCNT)      //determine Connect Key
        break;

#if LED_En
      ledAlt();                 //toggle LED 
#endif

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

#endif  //LEVELMODE
  //every interval send a "C",waiting XMODEM control command <soh>
  cnt = TimeOutCntC;
  while(1)
  {
    if(TIFRREG & (1 << OCF1A))  //T1 overflow
    {
      TIFRREG |= (1 << OCF1A);
      sendByte(XMODEM_RWC) ;    //send "C"

#if LED_En
      ledAlt();                 //toggle LED
#endif

      cnt--;
      if(cnt == 0)              //timeout
      {
        quitToUserApplication();                 //quit bootloader
      }
    }

#if WDG_En
    wdt_reset();                //clear watchdog
#endif

    if(dataInCom())
    {
      //if(ReadCom() == XMODEM_SOH)  //XMODEM command <soh>
        break;
    }
  }

  TCCR1B = 0;                   //close timer1

#if Decrypt
  DecryptInit();
#endif

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
    #if    (CRCMODE == 0)
    crch = readUARTData();                       //get CRC
    crcl = readUARTData();
    #elif  (CRCMODE == 1)
    checksum =  readUARTData();                  //get check sum
    #endif
	if ((packNO == RecivedPacketNo) && (packNO == PacketNoComplement)) 
    {
	  calculateCRC(&buf[bufptr - BUFFERSIZE]);       //calculate checksum
      #if   (CRCMODE  == 0)
      if((crch == ch) && (crcl == cl))
      #elif (CRCMODE == 1)
      if(checksum == cl) 
      #endif
	  {
#if BootStart
  		if(FlashAddr < BootStart)             //avoid write to boot section
        {
#endif

#if Decrypt
		  DecryptBlock(&buf[bufptr - BUFFERSIZE], BUFFERSIZE); //decrypt buffer
#endif

#if (BUFFERSIZE <= SPM_PAGESIZE)
          
          if(bufptr >= SPM_PAGESIZE)          //Flash page full, write flash page;otherwise receive next frame
          {                                   //receive multi frames, write one page
            writeOneFlashPage(buf);              //write data to Flash
            FlashAddr += SPM_PAGESIZE;        //modify Flash page address
            bufptr = 0;
          }
#else
          while(bufptr > 0)                   //receive one frame, write multi pages
          {
            writeOneFlashPage(&buf[BUFSIZE - bufptr]);
            FlashAddr += SPM_PAGESIZE;        //modify Flash page address
            bufptr -= SPM_PAGESIZE;
          }
#endif

#if BootStart
        }
        else                                  //ignore flash write when Flash address exceed BootStart
        {
          bufptr = 0;                         //reset receive pointer
        }
#endif
//read flash, and compare with buffer's content
#if (ChipCheck > 0) && (BootStart > 0)
#if (BUFFERSIZE < SPM_PAGESIZE)
        if((bufptr == 0) && (FlashAddr < BootStart))
#else
        if(FlashAddr < BootStart)
#endif
        {
          boot_rww_enable();                  //enable application section
          cl = 1;                             //clear error flag
          for(pagptr = 0; pagptr < BUFSIZE; pagptr++)
          {
#if (FLASHEND > 0xFFFFUL)
            if(pgm_read_byte_far(FlashAddr - BUFSIZE + pagptr) != buf[pagptr])
#else
            if(pgm_read_byte(FlashAddr - BUFSIZE + pagptr) != buf[pagptr])
#endif
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
            FlashAddr -= BUFSIZE;             //modify Flash page address
          }
        }
        else                                  //don't need verify, send ACK directly
        {
          sendByte(XMODEM_ACK);
          cnt = 0;
        }
#else
        sendByte(XMODEM_ACK);                 //no verify, send ACK directly
        cnt = 0;
#endif

#if WDG_En
        wdt_reset();                          //clear watchdog
#endif

#if LED_En
        ledAlt();                             //LED indicate update status
#endif
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
#if VERBOSE
  if(cnt == 0)
  {
    putstr(msg4);                             //prompt update success
  }
  else
  {
    // update fail
    putstr(msg5);                             //prompt update fail

#if WDG_En
    while(1);                                //dead loop, wait watchdog reset
#endif

  }

#else

#if WDG_En
  if(cnt > 0)
    while(1);                                //when update fail, use dead loop wait watchdog reset
#endif

#endif
  quitToUserApplication();                                     //quit bootloader
  return 0;
}

//End of file: bootldr.c
