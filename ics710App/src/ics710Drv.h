/*Yong Hu: 02-08-2010*/

#ifndef ICS710_DRV_H
#define ICS710_DRV_H

#include <dbScan.h>
#include <epicsMutex.h>
#include <epicsEvent.h>

#include "ics710api.h"

#define MAX_CHANNEL 32
#define MAX_DEV 8
#define MAX_SAMPLE 1<<18 /* 2^28 =  256K = 262144 */

#undef ics710Debug
/* #define ICS710IOC_DEBUG */
#ifdef ICS710IOC_DEBUG
#define ics710Debug(fmt, args...) printf(fmt, ## args)
#else
#define ics710Debug(fmt, args...)
#endif

struct ics710Driver {
  HANDLE hDevice; /*handler for individual card*/
  ICS710_CONTROL control; /*control register*/
  ICS710_GAIN gainControl;/*gain register*/
  ICS710_FILTER filterControl;
  ICS710_MASTER_CONTROL masterControl;
  ICS710_STATUS stat;
  unsigned totalChannel;
  unsigned long long channelCount; /*must be ULONGLONG*/
  unsigned nSamples; /* number of samples per channel*/
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

/*don't use 'extern' to define global variables which will be used in both C and C++:
 *startTime will be used ics710Daq.cpp and ics710Asbu.c
 * */
#ifdef __cplusplus
extern "C"
{
#endif
	//extern epicsTimeStamp startTime;
	//extern ics710Driver ics710Drivers[MAX_DEV];
	//extern epicsTime startTime;
#ifdef __cplusplus
}
#endif

#endif //#ifndef ICS710_DRV_H
