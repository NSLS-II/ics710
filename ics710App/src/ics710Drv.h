/*Yong Hu: 02-08-2010
 * Prototype IOC fully functions on 03-03-2011
 * */
/* ics710Drv.h: the most important data structure is ics710Driver*/

#ifndef ICS710_DRV_H
#define ICS710_DRV_H

#include <dbScan.h>
#include <epicsMutex.h>
#include <epicsEvent.h>

/*vendor(GE-IP) Linux Device Driver: kernel module ics710.ko
 * and API static library libics710.a
 * */
#include "ics710api.h"

const unsigned int MAX_CHANNEL = 32;
const unsigned int MAX_DEV = 8;
const unsigned int MAX_SAMPLE = 1 << 18; // 2^28 =  256K = 262144

#ifdef __cplusplus
extern "C"
{
#endif

#undef ics710Debug
/* #define ICS710IOC_DEBUG */
#ifdef ICS710IOC_DEBUG
#define ics710Debug(fmt, args...) printf(fmt, ## args)
#else
#define ics710Debug(fmt, args...)
#endif

/*the most important data structure for ics710 IOC:
 * all data associated with individual card
 * */
struct ics710Driver
{
    HANDLE hDevice;//handler for individual card
    int module;//#card
    //control register: trigger, acquisition mode, osr(over-sampling ratio),etc.
    ICS710_CONTROL control;
    ICS710_GAIN gainControl;//gain/input range register
    ICS710_FILTER filterControl; //filter/bandwidth register
    ICS710_MASTER_CONTROL masterControl; //master control register
    ICS710_STATUS stat;//status register: check if ADC calibration is complete
    unsigned int totalChannel;
    unsigned long long channelCount;//must be ULONGLONG
    unsigned int nSamples;//number of samples per channel
    unsigned long long acqLength;//acquisition length
    unsigned long long bufLength;//buffer length
    unsigned int bytesToRead;//DMA memory(buffer) size
    long *pAcqData;//pointer to DMA buffer
    double ics710AdcClockRate;
    double actualADCRate;
    double sampleRate;
    IOSCANPVT ioscanpvt;//I/O Intr
    epicsEventId runSemaphore;
    epicsMutexId daqMutex;
    unsigned int running;//start/stop DAQ
    unsigned int count;
    unsigned int timeouts;
    unsigned int readErrors;
    int rawData[MAX_CHANNEL][MAX_SAMPLE];//raw data (integer) for each channel
    double voltData[MAX_CHANNEL][MAX_SAMPLE];//voltage data for each channel
    double trigRate;
};

//global variable: one ics710Driver data structure per card
extern ics710Driver ics710Drivers[MAX_DEV];

#ifdef __cplusplus
}
#endif

#endif //#ifndef ICS710_DRV_H
