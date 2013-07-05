// Serial communication for raspberry
// Setup port and all


#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "serial.hpp"
#include <time.h>

int uart0_filestream;
char buf[255];
struct sigaction saio; 
//static struct serialDataStructure data;



void *serialThread(void* arg)
{
	int status = 0;
	long int start_time;
	long int time_difference;
	struct timespec gettime_now;
	int run = 1;
	int res = 0;

	int bytesReadFromBuffer = 0;

	struct serialDataStructure *serialCommData;

	serialCommData = (struct serialDataStructure *) arg;

	serialComm sc;

	sc.initialize();

	// Store initial time
	clock_gettime(CLOCK_REALTIME, &gettime_now);
	start_time = gettime_now.tv_nsec;		//Get nS value

	// Do some delay to stabilize serial port - ca. 100 msec
	// Because oxFF gets sent on port open and that seems to mess up receive process
	// on autopilot
	time_difference = 0;
	while(time_difference < 100000000)
	{
		clock_gettime(CLOCK_REALTIME, &gettime_now);
		time_difference = gettime_now.tv_nsec - start_time;
		if (time_difference < 0)
			time_difference += 1000000000;		//(Rolls over every 1 second)
	}


	sc.EnableConstantTransmission();
	// Enable constant transmission


	while(run)
	{
		// Keep time
		// If it takes too long to receive message, reset process state
		clock_gettime(CLOCK_REALTIME, &gettime_now);
		time_difference = gettime_now.tv_nsec - start_time;
		if (time_difference < 0)
			time_difference += 1000000000;		//(Rolls over every 1 second)
		if (time_difference > 500000000)		// If time difference over 500 msec
		{
			// Error
			sc.setIdle();
		}
		// Read out data
		unsigned char data[5120];
		int dataCount = 0;

		res = sc.serialDataAvail(uart0_filestream);

		if((res != 0)&&(res != -1))
		{
			bytesReadFromBuffer = read (uart0_filestream, &data, res);

			for(dataCount = 0; dataCount < bytesReadFromBuffer; dataCount++)
			{
				sc.MBMasterRecieveProcess(data[dataCount]);
			}
			if(sc.MBMasterData.data.ucDataReady)
			{
				res = 0;
				sc.waitingData = 0;
				sc.setIdle();
				sc.serialFlush(uart0_filestream);
				serialCommData->data[0] = sc.MBMasterData.data.uiData[71];
				serialCommData->data[1] = sc.MBMasterData.data.uiData[72];
				serialCommData->data[2] = sc.MBMasterData.data.uiData[73];
				serialCommData->data[3] = sc.MBMasterData.data.uiData[74];
				serialCommData->data[4] = sc.MBMasterData.data.uiData[75];
				serialCommData->status = 1;
				// Store end time = start time for next transmission
				clock_gettime(CLOCK_REALTIME, &gettime_now);
				start_time = gettime_now.tv_nsec;
				sc.MBMasterData.data.ucDataReady = 0;
			}
		}
	}
	return (void *)status;
}



serialComm::serialComm(void)
{
	// Initialize CRC tables
	unsigned char ucCRCTableHiTemp[] = {
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
		0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
		0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
		0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
		0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
		0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
		0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
		0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
		0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40
	} ;
	for(int i = 0; i < 256; i++)
	{
		ucCRCTableHi[i] = ucCRCTableHiTemp[i];
	}

	unsigned char ucCRCTableLoTemp[] = {
		0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
		0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
		0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
		0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
		0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
		0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
		0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
		0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
		0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
		0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
		0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
		0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
		0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
		0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
		0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
		0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
		0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
		0x40
	} ;
	for(int i = 0; i < 256; i++)
	{
		ucCRCTableLo[i] = ucCRCTableLoTemp[i];
	}

	iMBMTimeout = 100000;



	uart0_filestream = -1;
	isInitialized = 0;
}

void serialComm::setIdle()
{
	uiMBMasterProcessState = MBM_IDLE;
}

void serialComm::resetInterface(void)
{
	uiMBMasterProcessState = MBM_IDLE;
	MBMasterData.data.ucDataReady = 0;
	waitingData = 0;
	MBM_DataOK = 0;
	tcflush(uart0_filestream, TCIOFLUSH);
}

void serialComm::initialize(void)
{
	//-------------------------
	//----- SETUP USART 0 -----
	//-------------------------
	//At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
	uart0_filestream = -1;

	//OPEN THE UART
	//The flags (defined in fcntl.h):
	//	Access modes (use 1 of these):
	//		O_RDONLY - Open for reading only.
	//		O_RDWR - Open for reading and writing.
	//		O_WRONLY - Open for writing only.
	//
	//	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	//											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	//											immediately with a failure status if the output can't be written immediately.
	//
	//	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
	uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);		//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}

	//fcntl(uart0_filestream, F_SETFL, FNDELAY);
	/* allow the process to receive SIGIO */
	//fcntl(uart0_filestream, F_SETOWN, getpid());
	/* Make the file descriptor asynchronous (the manual page says only 
	O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
	//fcntl(uart0_filestream, F_SETFL, FASYNC);
	//fcntl(uart0_filestream, F_SETFL,  O_ASYNC );

	//CONFIGURE THE UART
	//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)
	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B38400 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = 0;// &= ~(IXON | IXOFF | IXANY);
	options.c_oflag = 0;// &=~ OPOST;
	options.c_lflag = 0;// &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_cc[VTIME]    = 0;   /* inter-character timer unused */
	options.c_cc[VMIN]     = 0;   /* blocking read until 0 chars received */
	tcflush(uart0_filestream, TCIOFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);
	resetInterface();
	isInitialized = 1;
}

void serialComm::sendData(unsigned char *tx_buffer, int bytes)
{	
	if (uart0_filestream != -1)
	{
		// Flush buffers
		serialFlush(uart0_filestream);
		sendingData = 1;
		/**
		int count = write(uart0_filestream, &tx_buffer, bytes);		//Filestream, bytes to write, number of bytes to write
		if (count < bytes)
		{
		printf("UART TX error\n");
		}*/

		for(int i = 0; i < bytes; i++)
		{
			int count = write(uart0_filestream, &tx_buffer[i], 1);		//Filestream, bytes to write, number of bytes to write
			if (count < 0)
			{
				printf("UART TX error\n");
			}
		}

		sendingData = 0;
		waitingData = 1;
	}
}

int serialComm::serialDataAvail (int fd)
{
	int result ;

	if (ioctl (fd, FIONREAD, &result) == -1)
		return -1 ;

	return result ;
}

/*
* serialFlush:
* Flush the serial buffers (both tx & rx)
*********************************************************************************
*/

void serialComm::serialFlush (int fd)
{
	tcflush (fd, TCIOFLUSH) ;
}

int serialComm::serialGetchar (int fd)
{
	unsigned int x ;

	if (read (fd, &x, 1) != 1)
		return -1 ;

	return ((int)x) & 0xFF ;
}

void serialComm::EnableConstantTransmission(void)
{
	//build message
	MBMasterData.bytes.cdata[0] = 0x33;									//ID = 0x33
	MBMasterData.bytes.cdata[1] = MBRWMULTIPLEREGISTERS;					//function code
	MBMasterData.bytes.cdata[2] = 0;			//start address hi
	MBMasterData.bytes.cdata[3] = 0;			//start address lo = 65
	MBMasterData.bytes.cdata[4] = 0;			//quant read hi
	MBMasterData.bytes.cdata[5] = 1;			//quant read lo
	MBMasterData.bytes.cdata[6] = 0;			//write start hi
	MBMasterData.bytes.cdata[7] = 0;			//write start lo
	MBMasterData.bytes.cdata[8] = 0;			//write quant hi
	MBMasterData.bytes.cdata[9] = 1;			//write quant lo
	MBMasterData.bytes.cdata[10] = 2;			//byte count
	MBMasterData.bytes.cdata[11] = 0;			//Command high
	MBMasterData.bytes.cdata[12] = 0;			//Command low
	MBMasterData.bytes.uiDataCount = 13;
	//calculate CRC
	MBMasterCRCOnMessage(0, MBMasterData.bytes.uiDataCount);
	MBMasterSendData();	//send data
	ucSlaveID = 0x33;
}

//function master read write multiple registers
void serialComm::MBMasterReadWriteMultipleRegisters(unsigned char slaveID, unsigned short readStart, unsigned short quantRead, unsigned short writeStart, unsigned short writeData[], unsigned short quantToWrite)
{
	int i = 0;
	//build message
	MBMasterData.bytes.cdata[0] = slaveID;									//ID = 0x33
	MBMasterData.bytes.cdata[1] = MBRWMULTIPLEREGISTERS;					//function code
	MBMasterData.bytes.cdata[2] = (readStart >> 8) & 0xFF;		//start address hi
	MBMasterData.bytes.cdata[3] = (readStart & 0xFF);			//start address lo = 65
	MBMasterData.bytes.cdata[4] = (quantRead >> 8) & 0xFF;		//quant read hi
	MBMasterData.bytes.cdata[5] = (quantRead & 0xFF);			//quant read lo
	MBMasterData.bytes.cdata[6] = (writeStart >> 8) & 0xFF;		//write start hi
	MBMasterData.bytes.cdata[7] = (writeStart & 0xFF);			//write start lo
	MBMasterData.bytes.cdata[8] = (quantToWrite >> 8) & 0xFF;	//write quant hi
	MBMasterData.bytes.cdata[9] = (quantToWrite & 0xFF);		//write quant lo
	// Write byte count
	MBMasterData.bytes.cdata[10] = (unsigned char)(quantToWrite * 2);			//write start lo
	MBMasterData.bytes.uiDataCount = 11;
	// Write all data
	for(i = 0; i < quantToWrite; i++)
	{
		MBMasterData.bytes.cdata[MBMasterData.bytes.uiDataCount] = (writeData[i] >> 8) & 0xFF;	//write quant hi
		MBMasterData.bytes.cdata[MBMasterData.bytes.uiDataCount + 1] = (writeData[i] & 0xFF);		//write quant lo
		MBMasterData.bytes.uiDataCount += 2;
	}

	//calculate CRC
	MBMasterCRCOnMessage(0, MBMasterData.bytes.uiDataCount);

	MBMasterSendData();	//send data

	ucSlaveID = slaveID;
}

//function master read registers
void serialComm::MBMasterReadRegisters(unsigned short slaveID, unsigned short startAddress, unsigned short readCount)
{
	//build message
	MBMasterData.bytes.cdata[0] = (unsigned char)slaveID;									//ID
	MBMasterData.bytes.cdata[1] = MBREADHOLDINGREGISTERS;					//function code
	MBMasterData.bytes.cdata[2] = (unsigned char)((startAddress >> 8) & 0x00ff);	//start address
	MBMasterData.bytes.cdata[3] = (unsigned char)(startAddress & 0x00ff);			//start address
	MBMasterData.bytes.cdata[4] = (unsigned char)((readCount >> 8) & 0x00ff);	//bytes count
	MBMasterData.bytes.cdata[5] = (unsigned char)(readCount & 0x00ff);			//bytes count
	MBMasterData.bytes.uiDataCount = 6;
	//calculate CRC
	MBMasterCRCOnMessage(0, MBMasterData.bytes.uiDataCount);
	MBMasterSendData();	//send data
	ucSlaveID = (unsigned char)slaveID;
}

//function master recieve response process
void serialComm::MBMasterRecieveProcess(unsigned int data)
{
	//iMBTimeout = 0;					//reset timeout
	MBMasterCRCOnByte(data);		//do CRC check
	switch(uiMBMasterProcessState)
	{
	case MBM_IDLE:
		{
			if(data == ucSlaveID)
			{
				iMBMResponseTimeout = 0;	//reset response timeout
				//MBM_MESSAGESENT = 0;		//mark response recieved

				ucMBMasterCRCHI = 0xFF ;  	// high byte of CRC initialized 
				ucMBMasterCRCLOW = 0xFF ;  // low byte of CRC initialized 
				MBMasterData.bytes.uiDataIndex = 0;
				MBMasterCRCOnByte(data);
				MBMasterData.bytes.cSlaveID = (char)data;
				uiMBMasterProcessState = MBM_RECEIVINGFCODE;
			}
			break;
		}	
	case MBM_RECEIVINGFCODE:
		{
			MBMasterData.bytes.cFunctionCode = (char)data;
			switch(MBMasterData.bytes.cFunctionCode)
			{
			case MBRWMULTIPLEREGISTERS:
				{
					uiMBMasterProcessState = MBM_RECEIVINGBYTECOUNT;
					break;
				}
			default:
				{
					uiMBMasterProcessState = MBM_IDLE;
					break;
				}
			}
			break;
		}
	case MBM_RECEIVINGWRITESTART_HI:
		{
			MBMasterData.bytes.uiWriteStartingAddressHi = (unsigned char)data;
			uiMBMasterProcessState = MBM_RECEIVINGWRITESTART_LO;
			break;
		}

	case MBM_RECEIVINGWRITESTART_LO:
		{
			MBMasterData.bytes.uiWriteStartingAddressLo = (unsigned char)data;
			uiMBMasterProcessState = MBM_RECEIVINGQWRITE_HI;
			break;
		}

	case MBM_RECEIVINGQWRITE_HI:
		{
			MBMasterData.bytes.uiQuantToWriteHi = (unsigned char)data;
			uiMBMasterProcessState = MBM_RECEIVINGQWRITE_LO;
			break;
		}

	case MBM_RECEIVINGQWRITE_LO:
		{
			MBMasterData.bytes.uiQuantToWriteLo = (unsigned char)data;
			uiMBMasterProcessState = MBM_RECEIVINGCRC_HI;
			break;
		}

	case MBM_RECEIVINGBYTECOUNT:
		{
			MBMasterData.bytes.ucWriteByteCount = (unsigned char)data;
			uiMBMasterProcessState = MBM_RECEIVINGDATA_HI;
			MBMasterData.data.uiDataIndex = 0;
			MBMasterData.bytes.uiDataCount = 0;
			break;
		}
	case MBM_RECEIVINGDATA_HI:
		{
			MBMasterData.bytes.cdata[MBMasterData.bytes.uiDataIndex+1] = (unsigned char)data;
			uiMBMasterProcessState = MBM_RECEIVINGDATA_LO;
			break;
		}
	case MBM_RECEIVINGDATA_LO:
		{
			MBMasterData.bytes.cdata[MBMasterData.bytes.uiDataIndex] = (unsigned char)data;
			MBMasterData.bytes.uiDataIndex += 2;
			MBMasterData.bytes.uiDataCount++;
			if(MBMasterData.bytes.uiDataIndex == MBMasterData.bytes.ucWriteByteCount)
			{
				//if all data recieved
				uiMBMasterProcessState = MBM_RECEIVINGCRC_HI;
			}
			else
			{
				uiMBMasterProcessState = MBM_RECEIVINGDATA_HI;
			}			
			break;
		}
	case MBM_RECEIVINGCRC_HI:
		{
			MBMasterData.bytes.uiCRCHi = (unsigned char)data;
			uiMBMasterProcessState = MBM_RECEIVINGCRC_LO;
			break;
		}
	case MBM_RECEIVINGCRC_LO:
		{			
			MBMasterData.bytes.uiCRCLo = (unsigned char)data;
			uiMBMasterProcessState = MBM_IDLE;
			if(MBMasterData.bytes.cFunctionCode == MBWRITEMULTIPLEREGISTERS)
			{

			}
			if((ucMBMasterCRCHI == 0)&&(ucMBMasterCRCLOW == 0))
			{
				MBMasterData.data.ucDataReady = 1;
				MBM_DataOK = 1;
			}
			else
			{
				uiMBMasterProcessState = MBM_IDLE;
				waitingData = 0;
				MBM_DataOK = 0;
			}
			break;
		}

	default:
		{
			uiMBMasterProcessState = MBM_IDLE;
			break;
		}	
	}
}

//function master process recieved response
void serialComm::MBMasterExecute(void)
{
	int iTemp = 0;
	//check if it is the right message
	//MBMasterQueue[MBMasterQueueIndex].data.
	//CRC is allready OK
	//check slave ID
	if(MBMasterData.bytes.cSlaveID == MBMLastCommand.data.cSlaveID)
	{
		//check f code
		if(MBMasterData.bytes.cFunctionCode == MBMLastCommand.data.cFunctionCode)
		{
			iTemp = 1;//success
			if(MBMLastCommand.data.cFunctionCode == MBWRITEMULTIPLEREGISTERS)
			{

			}
		}
		else
		{

		}	
	}
	else
	{

	}
	if(iTemp == 0)	//if message failed
	{

	}
	else	//if message is OK
	{
		switch(MBMasterData.bytes.cFunctionCode)
		{
		case MBREADHOLDINGREGISTERS:
			{
				break;
			}
		case MBWRITESINGLEREGISTER:
			{
				break;
			}
		case MBWRITEMULTIPLEREGISTERS:
			{
				break;
			}
		}
		//uiMBMasterInterval = 0;				//reset interval counter
	}
}

//function master send data
void serialComm::MBMasterSendData(void)
{
	//store last transmission
	MBMLastCommand.data.cSlaveID = (char)MBMasterData.bytes.cdata[0];
	MBMLastCommand.data.cFunctionCode = (char)MBMasterData.bytes.cdata[1];
	//send
	sendData(MBMasterData.bytes.cdata, MBMasterData.bytes.uiDataCount);
}

//function master do CRC check on recieved byte
void serialComm::MBMasterCRCOnByte(unsigned int ucMsgByte)
{
	unsigned int uiIndex = 0;
	uiIndex = (unsigned int)(ucMBMasterCRCHI^ucMsgByte) ; //calculate the CRC
	ucMBMasterCRCHI = (unsigned char) (ucMBMasterCRCLOW ^ ucCRCTableHi[uiIndex]) ;
	ucMBMasterCRCLOW = ucCRCTableLo[uiIndex] ;
}

//function master do CRC check on message
void serialComm::MBMasterCRCOnMessage(int uiDataIndex,unsigned int uiDataLen)
{
	unsigned int i = 0;
	unsigned int uiIndex = 0;
	ucMBMasterCRCHI = 0xFF ;  	// high byte of CRC initialized 
	ucMBMasterCRCLOW = 0xFF ;  // low byte of CRC initialized 

	uiIndex = 0;
	for(i=0;i < uiDataLen;i++)
	{
		uiIndex = (unsigned int)(ucMBMasterCRCHI^MBMasterData.bytes.cdata[i]) ; // calculate the CRC
		ucMBMasterCRCHI = (unsigned char) (ucMBMasterCRCLOW ^ ucCRCTableHi[uiIndex]) ;
		ucMBMasterCRCLOW = ucCRCTableLo[uiIndex] ;
	}
	MBMasterData.bytes.cdata[MBMasterData.bytes.uiDataCount] = ucMBMasterCRCHI;
	MBMasterData.bytes.uiDataCount ++;
	MBMasterData.bytes.cdata[MBMasterData.bytes.uiDataCount] = ucMBMasterCRCLOW;
	MBMasterData.bytes.uiDataCount ++;
}