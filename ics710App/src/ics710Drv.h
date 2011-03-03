/*Yong Hu: 02-08-2010*/

#ifndef ICS710_DRV_H
#define ICS710_DRV_H

#include <dbScan.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>

#include "ics710api.h"

#define MAX_CHANNEL 32
#define MAX_DEV 8
#define MAX_SAMPLE 1<<18 // 2^28 =  256K = 262144

#undef ics710Debug
//#define ICS710IOC_DEBUG
#ifdef ICS710IOC_DEBUG
#define ics710Debug(fmt, args...) printf(fmt, ## args)
#else
#define ics710Debug(fmt, args...)
#endif

struct ics710Driver {
 // unsigned module;
  HANDLE hDevice;
  ICS710_CONTROL control;
  ICS710_GAIN gainControl;
  ICS710_FILTER filterControl;
  ICS710_MASTER_CONTROL masterControl;
  ICS710_STATUS stat;
  unsigned totalChannel;
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
  unsigned truncated;
};
//typedef struct ics710Driver ad_t;

extern "C"
{
	extern ics710Driver ics710Drivers[MAX_DEV];
	//extern unsigned nbrIcs710Drivers;
	//extern epicsMutexId ics710DmaMutex;
	//extern int releaseBoard(ics710Driver *pics710Driver);
	extern int ics710Config(ics710Driver *pics710_driver);

}

#endif //#ifndef ICS710_DRV_H
