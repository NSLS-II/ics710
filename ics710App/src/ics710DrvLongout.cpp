/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */
/* ics710DrvLongout.cpp: used by ics710DevLongout.cpp; reconfigure parameters: totalChannel, samples/ch, samplingRate
 * BUT setTotalChannel and setSamples seem not work well because of re-allocation of DMA memory and reconfiguration of other parameters
 * */

#include "ics710Dev.h"
#include "ics710Drv.h"

#include <longoutRecord.h>
#include <epicsEvent.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LONGOUT_FUNC 3
/*global variable: defined in ics710DrvInit.cpp*/
extern  ics710Driver ics710Drivers[MAX_DEV];

static int setTotalChannel(longoutRecord* plongout, ics710Driver *pics710Driver, int val)
{
	int errorCode;
	/*2011-04-30: reconfiguration of totalChannel is a expensive operation including ADC recalibration, etc.
	 * just fix totalChannel = 8 */
	plongout->val = pics710Driver->totalChannel;
	printf("Sorry, totoalChannel is fixed to %d \n", pics710Driver->totalChannel);
	return 0;

	if (val == pics710Driver->totalChannel) return 0;

	if (pics710Driver->control.packed_data == 0)
		pics710Driver->bufLength = (val * pics710Driver->nSamples / 2) - 1;
	else
		pics710Driver->bufLength = (val * pics710Driver->nSamples / 4) - 1;
	if (pics710Driver->bufLength > 262143)
	{
		pics710Driver->bufLength = 262143;
		if (pics710Driver->control.packed_data == 0)
			val =  (262144 * 2) / pics710Driver->totalChannel;
		else
			val =  (262144 * 4) / pics710Driver->totalChannel;
		printf ("error(ics710DrvLongout.cpp: setTotalChannel): Calculated buf_len > 256K(4 MBytes memory), set Max.bufLength to 262143 \n");
	}

	if (val%2)
	{
		val = val - 1;
		printf("total active channels should be even number, set totalChannel = %d \n", val);
	}
	if (val < 2)
	{
		printf("total active channels should be > = 1: set totalChannel = 2 \n");
		val = 2;
	}
	if (val > 32)
	{
		printf("total active channels should be < = 32: set totalChannel = 32 \n");
		val = 32;
	}

	if (pics710Driver->control.packed_data == 0)
		pics710Driver->bufLength = (val * pics710Driver->nSamples / 2) - 1;
	else
		pics710Driver->bufLength = (val * pics710Driver->nSamples / 4) - 1;

	pics710Driver->totalChannel = val;
	if (pics710Driver->control.packed_data == 0)
		pics710Driver->channelCount = pics710Driver->totalChannel - 1; /* Channel Count Register, for un-packed data */
	else
		pics710Driver->channelCount = pics710Driver->totalChannel / 2 - 1; /* Channel Count Register, for packed data */

	pics710Driver->masterControl.numbers_of_channels = pics710Driver->channelCount;/* For Single Board, the two always equal */

	/*free the previous DMA memory and then re-allocate it*/
	//epicsMutexLock(pics710Driver->daqMutex);
	if (ics710FreeDmaBuffer (pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead))
	{
		printf ("error(ics710DrvLongout.cpp: setTotalChannle): can't free DMA memory \n");
		//epicsMutexUnlock(pics710Driver->daqMutex);
		return 0;
	}
	//epicsMutexUnlock(pics710Driver->daqMutex);
	pics710Driver->bytesToRead = (pics710Driver->bufLength + 1) * 8;  /* bytes to be read from ics-710 memory*/
	if (NULL == (pics710Driver->pAcqData = (long *) ics710AllocateDmaBuffer (pics710Driver->hDevice, pics710Driver->bytesToRead)))
	{
		printf ("error(ics710DrvLongout.cpp: setTotalChannle): can't re-allocate DMA memory for %u bytes\n",  pics710Driver->bytesToRead);
		return 0;
	}
	memset (pics710Driver->pAcqData, 0, pics710Driver->bytesToRead);

	if (ICS710_OK != (errorCode = ics710MasterControlSet (pics710Driver->hDevice, &(pics710Driver->masterControl))))
	{
		printf("can't set master control register(ics710DrvLongout.cpp), errorCode: %d \n", errorCode);
		return errorCode;
	}
	if (ICS710_OK != (errorCode = ics710ChannelCountSet (pics710Driver->hDevice, &(pics710Driver->channelCount))))
	{
		printf("can't set channel count register(ics710DrvLongout.cpp), errorCode: %d \n", errorCode);
		return errorCode;
	}
	if (ICS710_OK != (errorCode = ics710BufferReset (pics710Driver->hDevice)))
	{
		printf("can't reset buffer register(ics710DrvLongout.cpp: setTotalChannle), errorCode: %d \n", errorCode);
		return errorCode;
	}

	plongout->val = val;
	printf("reconfigure totoalChannel to %d channels \n", pics710Driver->totalChannel);
	return 0;
}

static int setSamples(longoutRecord* plongout, ics710Driver *pics710Driver, int val)
{
	int errorCode;

	if (val == pics710Driver->nSamples) return 0;

	epicsEventWait(pics710Driver->runSemaphore);
	if (val < 1)
	{
		printf("number of samples should be > = 1: set Samples = 1 \n");
		val = 1;
	}
	if (val > 262144)
	{
		printf("number of samples should be < = 262144: set Samples = 262144 \n");
		val = 262144;
	}

	if (pics710Driver->control.packed_data == 0)
		pics710Driver->bufLength = (val * pics710Driver->totalChannel / 2) - 1;
	else
		pics710Driver->bufLength = (val * pics710Driver->totalChannel / 4) - 1;
	if (pics710Driver->bufLength > 262143)
	{
		pics710Driver->bufLength = 262143;
		if (pics710Driver->control.packed_data == 0)
			val =  (262144 * 2) / pics710Driver->totalChannel;
		else
			val =  (262144 * 4) / pics710Driver->totalChannel;
		printf ("error(ics710DrvLongout.cpp: setSamples): Calculated buf_len > 256K(4 MBytes memory), set Max.bufLength to 262143 \n");
	}

	pics710Driver->nSamples = val;
	pics710Driver->acqLength = pics710Driver->nSamples - 1;
	if (pics710Driver->control.packed_data == 0)
		pics710Driver->bufLength = (pics710Driver->nSamples * pics710Driver->totalChannel / 2) - 1;
	else
		pics710Driver->bufLength = (pics710Driver->nSamples * pics710Driver->totalChannel / 4) - 1;

	/*free the previous DMA memory and then re-allocate it*/
	//epicsMutexLock(pics710Driver->daqMutex);
	if (ics710FreeDmaBuffer (pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead))
	{
		printf ("error(ics710DrvLongout.cpp: setSamples): can't free DMA memory \n");
		//epicsMutexUnlock(pics710Driver->daqMutex);
		return 0;
	}
	//epicsMutexUnlock(pics710Driver->daqMutex);
	pics710Driver->bytesToRead = (pics710Driver->bufLength + 1) * 8;  /* bytes to be read from ics-710 memory*/
	if (NULL == (pics710Driver->pAcqData = (long *) ics710AllocateDmaBuffer (pics710Driver->hDevice, pics710Driver->bytesToRead)))
	{
		printf ("error(ics710DrvLongout.cpp: setSamples): can't re-allocate DMA memory %u bytes\n",  pics710Driver->bytesToRead);
		return 0;
	}
	memset (pics710Driver->pAcqData, 0, pics710Driver->bytesToRead);

	if (ICS710_OK != (errorCode = ics710BufferLengthSet (pics710Driver->hDevice, &(pics710Driver->bufLength))))
	{
		printf("can't reconfigure buffer length register(ics710DrvLongout.cpp), errorCode: %d \n", errorCode);
		return errorCode;
	}
	if (ICS710_OK != (errorCode = ics710AcquireCountSet (pics710Driver->hDevice, &(pics710Driver->acqLength))))
	{
		printf("can't reconfigure acquisition length register(ics710DrvLongout.cpp), errorCode: %d \n", errorCode);
		return errorCode;
	}
	if (ICS710_OK != (errorCode = ics710BufferReset (pics710Driver->hDevice)))
	{
		printf("can't reset buffer register(ics710DrvLongout.cpp: setSamples), errorCode: %d \n", errorCode);
		return errorCode;
	}

	plongout->val = val;
	printf("reconfigure samples/channel to %d samples \n", pics710Driver->nSamples);
	epicsEventSignal(pics710Driver->runSemaphore);

	return 0;
}

static int setSamplingRate(longoutRecord* plongout, ics710Driver *pics710Driver, int val)
{
	int errorCode;

	if (abs((1000*pics710Driver->ics710AdcClockRate/(256.0/(1<<pics710Driver->control.oversamp_ratio)) -val)) < 1.00) return 0;

	pics710Driver->ics710AdcClockRate = val * (256.0/(1<<pics710Driver->control.oversamp_ratio))/1000.0;
	if (pics710Driver->ics710AdcClockRate < 0.256)
	{
		pics710Driver->ics710AdcClockRate = 0.256;
		val = pics710Driver->ics710AdcClockRate / ((256.0/(1<<pics710Driver->control.oversamp_ratio))/1000.0);
		printf("error: sampling rate is too low, set to the speed %d KHz \n", val);
	}
	if (pics710Driver->ics710AdcClockRate > 13.824)
	{
		pics710Driver->ics710AdcClockRate = 13.824;
		val = pics710Driver->ics710AdcClockRate / ((256.0/(1<<pics710Driver->control.oversamp_ratio))/1000.0);
		printf("error: sampling rate is too high, set to the speed %d KHz \n", val);
	}

	if (ICS710_OK != (errorCode = ics710ADCClockSet (pics710Driver->hDevice, &pics710Driver->ics710AdcClockRate, &pics710Driver->actualADCRate)))
	{
		printf("errorCode: can't reconfigure ADC clock in ics710DrvLongout.cpp/setSamplingRate %dKHz \n", errorCode);
		return errorCode;
	}
	plongout->val = val;
	printf("reconfigure data output rate by setSamplingRate: %d KHz \n", val);

	return 0;
}

/* choose the function according to LINK string name */
typedef int (*ics710LongoutFunc)(longoutRecord* plongout, ics710Driver *pics710Driver, int val);
struct ics710LongoutFuncStruct
{
	  ics710LongoutFunc wfunc;
};
static struct
{
  const char* name;
  ics710LongoutFunc wfunc;
} parseLongoutString[MAX_LONGOUT_FUNC] =
{
  {"LCHA", setTotalChannel},
  {"LSAM", setSamples},
  {"LRAT", setSamplingRate},
};

template<> int ics710InitRecordSpecialized(longoutRecord* plongout)
{
	  int i;
	  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(plongout->dpvt);
	  ics710Driver* pics710Driver = &ics710Drivers[pics710RecPrivate->card];
	  ics710Debug("ics710InitLongoutRecord: record name: %s, card: %d, link name: %s \n", plongout->name, pics710RecPrivate->card, pics710RecPrivate->name);

	  for (i = 0; i < MAX_LONGOUT_FUNC; i++)
	  {
	       if (0 == strcmp(pics710RecPrivate->name, parseLongoutString[i].name))
	       {
	    		  /* choose the function according to LINK string name */
		    	ics710LongoutFuncStruct* pics710LongoutFuncStruct = new ics710LongoutFuncStruct;
		    	pics710LongoutFuncStruct->wfunc = parseLongoutString[i].wfunc;
		        pics710RecPrivate->pvt = pics710LongoutFuncStruct;
		        ics710Debug("parseLongoutString[i].name: %s \n", parseLongoutString[i].name);
		        //return 0;
	       }

	       /* initialize the .VAL field using the setup parameters in st.cmd */
	       if (0 == strcmp(pics710RecPrivate->name, "LCHA"))
	       {
	    	   ics710ChannelCountGet(pics710Driver->hDevice, &(pics710Driver->channelCount));
	    	   plongout->val = 1 + pics710Driver->channelCount;
	    	   //printf("%s.val is: %d, totalCH: %d \n",plongout->name, plongout->val, 1+pics710Driver->channelCount);
	       }
	       if (0 == strcmp(pics710RecPrivate->name, "LSAM"))
	       {
	    	   ics710AcquireCountGet(pics710Driver->hDevice, &(pics710Driver->acqLength));
	    	   plongout->val = 1 + pics710Driver->acqLength;
	    	   //printf("%s.val is: %d, totalSamples: %d \n",plongout->name, plongout->val, 1+pics710Driver->acqLength);
	       }
	       if (0 == strcmp(pics710RecPrivate->name, "LRAT"))
	       {
	    	   ics710ADCClockSet(pics710Driver->hDevice, &pics710Driver->ics710AdcClockRate, &pics710Driver->actualADCRate);
	    	   plongout->val = 1000*pics710Driver->ics710AdcClockRate/(256.0/(1<<pics710Driver->control.oversamp_ratio));
	    	   //printf("%s sampleRate: %dKHz \n",plongout->name, plongout->val);
	       }

	  }//for (i = 0; i < MAX_LONGOUT_FUNC; i++)

	  return 0;
}

template<> int ics710WriteRecordSpecialized(longoutRecord* plongout)
{
	  int status;
	  int val;

	  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(plongout->dpvt);
	  ics710Driver* pics710Driver = &ics710Drivers[pics710RecPrivate->card];
	  ics710LongoutFuncStruct* pics710LongoutFuncStruct = reinterpret_cast<ics710LongoutFuncStruct*>(pics710RecPrivate->pvt);

	  val = plongout->val;
	  status = pics710LongoutFuncStruct->wfunc(plongout, pics710Driver, val);

	  return status;
}
