//var.h
#ifndef _VAR_H_
#define _VAR_H_

typedef union
{
	long pozicijalong;
	struct
	{
		unsigned int lower;
		unsigned int upper;
	}pozicijaInt;
}pozicija;



typedef struct tagFLAGBITS 
{
  union 
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
    };
    struct 
    {
      unsigned LOW:8;
      unsigned HIGH:8;
    };
  };
} FLAGBITS ;

extern FLAGBITS flag0;
extern FLAGBITS flag1;
extern FLAGBITS WMultiRegisterBits;
extern FLAGBITS statusControllReg;
extern unsigned int machineID;


extern unsigned int sCount;				//count 1 sec time
extern unsigned int uiToggleLedCounter;
extern int dir;
extern unsigned int uiSlaveRecievedCount;

extern unsigned int uiMBMasterInterval;

//X axis data
extern pozicija currentPositionX;
extern pozicija desiredPositionX;
extern FLAGBITS statusX;
extern unsigned int speedX;
extern unsigned int xRecievedCount;
extern unsigned int xSlaveID;

//Y axis data
extern pozicija currentPositionY;
extern pozicija desiredPositionY;
extern FLAGBITS statusY;
extern unsigned int speedY;

//Z axis data
extern pozicija currentPositionZ;
extern pozicija desiredPositionZ;
extern FLAGBITS statusZ;
extern unsigned int speedZ;

extern unsigned int uiSlaveRotation;

//servo data
extern unsigned int uiServoPosition;
extern unsigned int servoPulseWidth;		//control signal
extern unsigned int servoPulseTime;	//width of servo signal
extern int servoPulse;				//=20 ms counter

//Local stepper data
extern unsigned int uiStepperPosition;
extern unsigned int uiDesiredStepperPosition;
extern unsigned int uiStepperSpeed;
extern unsigned int uiStepperPeriod;
;

 //LCD data
extern unsigned int uiLCD_Data;			//holds data to be transmitted
extern  unsigned int uiLCDCounter;		//delay counter
extern unsigned int uiLCDTime;			//delay limit time
extern unsigned int uiLCDDisplay;			//what to display on LCD
extern unsigned int uiFinishCount;

#endif