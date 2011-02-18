/*Yong Hu: 02-08-2010*/

#ifndef ICS710_DRV_H
#define ICS710_DRV_H

#define MAX_CHANNEL 32
#define MAX_DEV 8
#define MAX_SAMPLE 1<<18 // 2^28 =  256K = 262144

#include <dbScan.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>

#include "ics710api.h"
/*
struct ics710_data_t {
  unsigned nsamples;
  void* buffer;
};
*/
struct ics710Driver {
  unsigned module;
  HANDLE hDevice;
  ICS710_CONTROL control;
  ICS710_GAIN gainControl;
  ICS710_FILTER filterControl;
  ICS710_MASTER_CONTROL masterControl;
  ICS710_STATUS stat;
  unsigned totalChannel;
  unsigned swapTimes;
  unsigned long long channelCount; /*must be ULONGLONG*/
  unsigned nSamples; /* Total sample number per channels*/
  unsigned long long acqLength;
  unsigned long long bufLength;
  unsigned bytesToRead;
  long *pAcqData;
  double ics710AdcClockRate;
  double actualADCRate;
  IOSCANPVT ioscanpvt;
  epicsEventId runSemaphore;
  epicsMutexId daqMutex;
  int running;
  unsigned count;
  unsigned timeouts;
  unsigned readErrors;
  double chData[MAX_CHANNEL][MAX_SAMPLE];
};
//typedef struct ics710Driver ad_t;

extern "C"
{
	extern ics710Driver ics710Drivers[MAX_DEV];
	//extern unsigned nbrIcs710Drivers;
	//extern epicsMutexId ics710DmaMutex;
	//extern int errorExit(int errorCode, ics710Driver *pics710Driver);
	//extern int releaseBoard(ics710Driver *pics710Driver);
	//extern int ics710Reconfig(ics710Driver *pics710Driver);
	//extern int ics710Config(ics710Driver *pics710_driver);

}

#define SUCCESS(x) (((x)&0x80000000) == 0)
/*
#define BOARD_RESET_ERROR		-100
#define DEFAULT_CLOCK_ERROR		-99
#define FPDP_CLOCK_ERROR		-98
#define ADC_CLOCK_ERROR			-97
#define CONTROL_ERROR			-96
#define MASTER_CONTROL_ERROR	-95
#define GAIN_ERROR				-94
#define FILTER_ERROR			-93
#define CHANNEL_COUNTL_ERROR	-92
#define BUFFER_LENGTH_ERROR		-91
#define ACQUIRE_COUNT_ERROR		-90
#define FPDP_FRAMES_ERROR		-89
#define DMA_LOCAL_SPACE_ERROR	-88
#define BUFFER_RESET_ERROR		-87
#define ADC_RESET_ERROR			-86
#define STATUS_ERROR			-85
#define TIMEOUT_ERROR			-84
#define ENABLE_ERROR			-83
*/
#endif //#ifndef ICS710_DRV_H
