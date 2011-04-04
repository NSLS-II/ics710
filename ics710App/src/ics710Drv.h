/*Yong Hu: 02-08-2010
 * Prototype IOC fully functions on 03-03-2011
 * */
/* ics710Drv.h: the most important data structure for ics710 IOC is ics710Driver*/

#ifndef ICS710_DRV_H
#define ICS710_DRV_H

#include <dbScan.h>
#include <epicsMutex.h>
#include <epicsEvent.h>

/*vendor(GE-IP) Linux Device Driver: kernel module ics710.ko and API static library libics710.a*/
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

/*the most important data structure for ics710 IOC*/
struct ics710Driver {
  HANDLE hDevice; /*handler for individual card*/
  ICS710_CONTROL control; /*control register: trigger, acquisition mode, osr(over-sampling ratio),etc.*/
  ICS710_GAIN gainControl;/*gain/input range register*/
  ICS710_FILTER filterControl; /*filter/bandwidth register*/
  ICS710_MASTER_CONTROL masterControl; /*master control register*/
  ICS710_STATUS stat; /*status register: check if ADC calibration is complete*/
  unsigned totalChannel;
  unsigned long long channelCount; /*must be ULONGLONG*/
  unsigned nSamples; /* number of samples per channel*/
  unsigned long long acqLength; /*acquisition length*/
  unsigned long long bufLength; /*buffer length*/
  unsigned bytesToRead; /*DMA memory(buffer) size*/
  long *pAcqData; /*pointer to DMA buffer*/
  double ics710AdcClockRate;
  double actualADCRate;
  IOSCANPVT ioscanpvt; /*I/O Intr*/
  epicsEventId runSemaphore;
  epicsMutexId daqMutex;
  int running; /*start/stop DAQ*/
  unsigned count;
  unsigned timeouts;
  unsigned readErrors;
  double chData[MAX_CHANNEL][MAX_SAMPLE]; /*voltage data for individual channel*/
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
