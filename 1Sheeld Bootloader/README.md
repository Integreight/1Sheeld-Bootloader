# 1Sheeld Bootloader

The bootloader in general is a normal firmware stored in a special section of the flash memory to be used later for updating firmware at program memory section in flash memory.1Sheeld Bootloader is the bootloader for 1Sheeld firmware where you can update your 1Sheeld firmware with your android smartphone.UART is used for communication.

## Documentation 

	https://bitbucket.org/IslamMustafa/1sheeld-bootloader/downloads/Deploying%20boot-loader%20on%20Atmega162.docx

## How to install

Clone the project into a new folder in your hard disk.

	git clone https://github.com/Integreight/1Sheeld-Arduino-Library.git ./OneSheeld

## Building

Open the project with ATmel Studio and build it, in the memory settings from the properties of the solution, you must set .text in the flash segment to be equal to the starting address of the boot-loader. e.g. If we are using the boot size to be 1024 words. There for we’ll put (.text=0x1C00). and then program your device as if it's ordinary hex file but you need to adjust the BOOTSZ1, BOOTSZ0 and BOOTRST fuse bits as shown : 
The BOOTRST must be set to 0 in order for program counter to start reading the boot-section. The BOOTSZx bits determine the size of the boot-loader.

BOOTSZ1  	BOOTSZ0  	Boot Size
1	        1	        128 words
1	        0	        256 words
0	        1	        512 words
0	        0	        1024 words
