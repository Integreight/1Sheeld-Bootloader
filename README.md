# 1Sheeld Bootloader [![Build Status](https://travis-ci.org/Integreight/1Sheeld-Bootloader.svg?branch=master)](https://travis-ci.org/Integreight/1Sheeld-Bootloader)#

## Overview ##

1Sheeld bootloader is a small piece of firmware that come pre-installed on your 1Sheeld board and is used mainly for flashing new firmware updates to the board using the app. 

## Introduction ##

1Sheelds microcontroller's (ATmega162) flash memory is divided into two sections, the boot section and the application section. Both of them contain the bootloader and the firmware respectively. 
 
Every time the board is reset using the reset button, it will start executing the bootloader from the boot section and wait for 200ms for a request from the app to update the firmware. If the request is received, the bootloader will communicate with the app, receive the new firmware and replace the old one, it will then switch to the application section and run the new firmware. If no request is received the bootloader will exit and execute the firmware found in the application section. 
 
## Communication Method ##

The bootloader communicates with the app through serial protocol on ATmega162's UART1. 
 
## File Format ##

The program file is transferred in the binary format. So if the file is in Intel hex format (.hex) you have to transform it to binary (.bin) format before flashing using a tool like [Hex2Bin](http://hex2bin.sourceforge.net/). 
 
## Communication Protocol ##

The bootloader uses the standard [XModem](http://en.wikipedia.org/wiki/XMODEM) protocol with [CRC16](http://en.wikipedia.org/wiki/Cyclic_redundancy_check) for error checking.

Here is a breakdown of the firmware upgrading steps: 
 
- The bootloader sends a NAK to notifies the app that it is ready for the password. 
- In less than 200ms, the app sends the 8 characters password : ``` {0x64, 0x0E, 0x1C, 0x39, 0x14, 0x28, 0x57, 0xAA} ```
- The bootloader sends 'C' character to notifies the app that it is using the CRC16 method. 
- The app splits the binary file into 128 bytes packets and transmit them frame by frame in this format: 

``` 
- an SOH byte                                  {1 byte} 
- the packet number                            {1 byte} 
- the 1's complement of the packet number      {1 byte} 
- the packet                                   {128 bytes} 
- the high byte of the CRC value               {1 byte} 
- the low byte of the CRC value                {1 byte} 
```

- If the bootloader finds that the CRC values matches the computed ones, it flashs the packet and sends an ACK otherwise it sends a NAK. 
- The app waits for an ACK to transmit the next frame or a NAK to resends the last one until the whole binary file is sent. 
- After the whole binary file is sent, the app sends an EOT. 
- If there are no errors, the bootloader sends an ACK. 

## Building ##

The repo is an Atmel Studio project, but we have plans to make it as a generic C project so you can build it easliy on platforms other than Microsoft Windows with a standard MakeFile. 

## Uploading ##

The ICSP pins are exposed with a 6-pin header on the bottom of your 1Sheeld board. You can easily connect any ATmega programmer and upload your own version of the bootloader.
 
## Fuse Bits ##

- Low Value: 0xFD
- High Value: 0xD8
- Extended Value: 0xFB

Click [here](http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega162&LOW=FD&HIGH=D8&EXTENDED=FB&LOCKBIT=CC) for a description of the enabled fuse bits.

## Contribution ##

Contributions are welcomed, please follow this pattern:
- Fork the repo.
- Open an issue with your proposed feature or bug fix.
- Commit and push code to a new branch in your forked repo.
- Submit a pull request to our *development* branch.

Don't forget to drop us an email, post on our forum, or mention us on Twitter or Facebook about what you have built using 1Sheeld, we would love to hear about it.

## Changelog ##

To see what has changed in recent versions of 1Sheeld Bootloader, see the [Change Log](CHANGELOG.md).

## Note ##

1Sheeld Bootloader is based on [AVR Universal Bootloader(AVRUB)](http://sourceforge.net/projects/avrub) by Shao Ziyang.
 
## License and Copyright ##

```
This code is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 3 only, as
published by the Free Software Foundation.

This code is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
version 3 for more details (a copy is included in the LICENSE file that
accompanied this code).

Please contact Integreight, Inc. at info@integreight.com or post on our
support forums www.1sheeld.com/forum if you need additional information
or have any questions.
```