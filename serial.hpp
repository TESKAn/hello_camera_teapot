#ifndef _SERIAL_H_
#define _SERIAL_H_

#define MB_TOTALBYTES	255

#define MBM_IDLE							0
#define MBM_RECEIVINGFCODE				1
#define MBM_RECEIVINGWRITESTART_HI		2
#define MBM_RECEIVINGWRITESTART_LO		3
#define MBM_RECEIVINGQWRITE_HI			4
#define MBM_RECEIVINGQWRITE_LO			5
#define MBM_RECEIVINGBYTECOUNT			6
#define MBM_RECEIVINGDATA_HI				7
#define MBM_RECEIVINGDATA_LO				8
#define MBM_RECEIVINGCRC_HI				9
#define MBM_RECEIVINGCRC_LO				10

//MB functions macros
#define MBRWMULTIPLEREGISTERS			23
#define MBWRITESINGLEREGISTER			6
#define MBREADHOLDINGREGISTERS			3
#define MBWRITEMULTIPLEREGISTERS		16
#define MBEXCEPTION						0x80
#define MBILLEGALFCODE					0x01
#define MBILLEGALDATAADDRESS			0x02


struct serialDataStructure
{
	unsigned short data[128];
	unsigned short status;
};


//last command structure
typedef union
{
	struct
	{
		unsigned char ucdata[2];
	}base;
	struct
	{
		char cSlaveID;
		 char cFunctionCode;
	}data;
}MBMasterLastCommandStructure;

//MB master write multiple register structure
typedef union
{
	struct
	{
		unsigned char ucdata[16];
	}base;
	struct	//16 bytes
	 { 	
		 char cSlaveID;
		 char cFunctionCode;
		 unsigned int uiWriteStartingAddress;	
		 unsigned int uiQuantToWrite;
		 unsigned char ucWriteByteCount;
		 unsigned char ucDataReady;
	 	 unsigned int uiData1;
	 	 unsigned int uiData2;
	 	 unsigned int uiData3;
	 	 unsigned int uiCRC;
	 	 unsigned char ucRetries;
	 	 unsigned char empty;
	 }data;
	 
	 struct	//16 bytes
	 {
	 	 char cSlaveID;
		 char cFunctionCode;
		 unsigned char ucWriteStartingAddressLo;	
		 unsigned char ucWriteStartingAddressHi;	
		 unsigned char ucQuantToWriteLo;
		 unsigned char ucQuantToWriteHi;
		 unsigned char ucWriteByteCount;
	 	 unsigned char ucDataReady;
	 	 unsigned char ucData1Lo;
	 	 unsigned char ucData1Hi;
	 	 unsigned char ucData2Lo;
	 	 unsigned char ucData2Hi;
	 	 unsigned char ucData3Lo;
	 	 unsigned char ucData3Hi;
	 	 unsigned char ucCRCLo;
	 	 unsigned char ucCRCHi;
	 	 unsigned char ucRetries;
	 	 unsigned char empty;
	 }bytes;
}MBMasterWriteMultiRegisterDataStructure;


// MODBUS data structure
typedef union
{
	struct	//MB_TOTALBYTES bytes
	{
		unsigned char ucdata[MB_TOTALBYTES];
	}base;

	struct	//MB_TOTALBYTES bytes
	{
		char cSlaveID:8;
		char cFunctionCode:8;
		unsigned int uiReadStartingAddress:16;
		unsigned int uiQuantToRead:16;
		unsigned int uiWriteStartingAddress:16;
		unsigned int uiQuantToWrite:16;
		unsigned char ucWriteByteCount:8;
		unsigned char ucDataReady:8;
		unsigned int uiDataIndex:16;
		unsigned int uiDataCount:16;
		unsigned int uiCRC:16;
		unsigned short uiData[(MB_TOTALBYTES - 18)/2];
	}data;

	struct	//MB_TOTALBYTES bytes
	{
		char cSlaveID:8;
		char cFunctionCode:8;
		unsigned char uiReadStartingAddressLo:8;
		unsigned char uiReadStartingAddressHi:8;
		unsigned char uiQuantToReadLo:8;
		unsigned char uiQuantToReadHi:8;
		unsigned char uiWriteStartingAddressLo:8;
		unsigned char uiWriteStartingAddressHi:8;
		unsigned char uiQuantToWriteLo:8;
		unsigned char uiQuantToWriteHi:8;
		unsigned char ucWriteByteCount:8;
		unsigned char ucDataReady:8;
		unsigned int uiDataIndex:16;
		unsigned int uiDataCount:16;
		unsigned char uiCRCLo:8;
		unsigned char uiCRCHi:8;
		unsigned char cdata[MB_TOTALBYTES - 18];
	}bytes;

} mbd;

typedef union
{
	unsigned int FLAG;
	struct
	{
		unsigned BIT0:1;	
		unsigned BIT1:1;
		unsigned BIT2:1;
		unsigned BIT3:1;
		unsigned BIT4:1;
		unsigned BIT5:1;
		unsigned BIT6:1;
		unsigned BIT7:1;
		unsigned BIT8:1;
		unsigned BIT9:1;
		unsigned BIT10:1;
		unsigned BIT11:1;
		unsigned BIT12:1;
		unsigned BIT13:1;
		unsigned BIT14:1;
		unsigned BIT15:1;
	}BITS;
}FLAGREGISTER;

class serialComm
{
private:
	// Mark transmission in progress
	int sendingData;
	
	// CRC tables
	unsigned char ucCRCTableHi[256];
	unsigned char ucCRCTableLo[256];
	//MODBUS variables
	volatile int MODBUS_ReceiveState;
	volatile unsigned char ucMBCRCLOW;
	volatile unsigned char ucMBCRCHI;

	int volatile MODBUS_Timeout;

	int iMBMTimeout;
	int iMBMResponseTimeout;
	int MBMResendDelayCounter;
	unsigned char ucSlaveID;
	unsigned int uiMBMasterBufferLocation;	//current location in buffer
	unsigned int uiMBMasterBufferCount;		//no. of bytes in buffer
	unsigned char ucMBMasterCRCLOW;
	unsigned char ucMBMasterCRCHI;
	unsigned int uiMBMasterProcessState;
	int iMBMasterTimeout;
	//FLAGREGISTER MBFLAG;
	//FLAGREGISTER MBRWFLAG1;
	//FLAGREGISTER MBRWFLAG2;
	
	FLAGREGISTER MBMASTERFLAG;
	MBMasterWriteMultiRegisterDataStructure MBMasterQueue[5];
	MBMasterWriteMultiRegisterDataStructure MBMCommandTemp;
	MBMasterLastCommandStructure MBMLastCommand;

 int MBMasterQueueIndex;						//index in queue
 int MBMasterQueueLast;						//Last command in queue


public:
	int waitingData;
	int baudRate;
	char isInitialized;
	mbd MBMasterData;
	unsigned char MBSlaveID;
	unsigned char MBM_DataOK;
	unsigned int expectedBytes;
	//void (*serialCallBack)(unsigned char *);
	serialComm();
	void setIdle();
	void resetInterface(void);
	void initialize(void (*callback) (int stat));
	void initialize(void);
	void sendData(unsigned char *tx_buffer, int bytes);
	int serialDataAvail (int fd);
	void serialFlush (int fd);
	int serialGetchar (int fd);
	void readData(unsigned char *rx_buffer, int *bytesRead);
	void EnableConstantTransmission(void);
	void MBMasterReadWriteMultipleRegisters(unsigned char slaveID, unsigned short readStart, unsigned short quantRead, unsigned short writeStart, unsigned short writeData[], unsigned short quantToWrite);
	void MBMasterReadRegisters(unsigned short slaveID, unsigned short startAddress, unsigned short readCount);
	void MBMasterRecieveProcess(unsigned int data);
	void MBMasterExecute(void);
	void MBMasterSendData(void);
	void MBMasterCRCOnMessage(int uiDataIndex,unsigned int uiDataLen);
	void MBMasterCRCOnByte(unsigned int ucMsgByte);
};

void *serialThread(void* arg);


#endif