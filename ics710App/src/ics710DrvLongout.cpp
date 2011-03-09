/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */

#include "ics710api.h"

#include "ics710Dev.h"
#include "ics710Drv.h"

//#include <epicsMutex.h>
#include <longoutRecord.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LONGOUT_FUNC 3

static int setTotalChannel(ics710Driver *pics710Driver, int val)
{
	int errorCode;
	if (val == pics710Driver->totalChannel) return 0;

	pics710Driver->totalChannel = val;

	if (pics710Driver->control.packed_data == 0)
		pics710Driver->channelCount = pics710Driver->totalChannel - 1; /* Channel Count Register, for un-packed data */
	else
		pics710Driver->channelCount = pics710Driver->totalChannel / 2 - 1; /* Channel Count Register, for packed data */

	pics710Driver->masterControl.numbers_of_channels = pics710Driver->channelCount;/* For Single Board, the two always equal */

	if (pics710Driver->control.packed_data == 0)
		pics710Driver->bufLength = (pics710Driver->totalChannel * pics710Driver->nSamples / 2) - 1;
	else
		pics710Driver->bufLength = (pics710Driver->totalChannel * pics710Driver->nSamples / 4) - 1;
	if (pics710Driver->bufLength > 262143)
	{
		printf ("error(ics710DrvLongout.cpp): Calculated buf_len > 256K(4 MBytes memory board), set to Max. 262143 \n");
		pics710Driver->bufLength = 262143;
	}

	/*free the previous DMA memory and then re-allocate it*/
	ics710FreeDmaBuffer (pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead);
	pics710Driver->bytesToRead = (pics710Driver->bufLength + 1) * 8;  /* bytes to be read from ics-710 memory*/
	pics710Driver->pAcqData = (long *) ics710AllocateDmaBuffer (pics710Driver->hDevice, pics710Driver->bytesToRead);
	if (NULL == pics710Driver->pAcqData)
	{
		printf ("error(ics710DrvLongout.cpp): Not enough memory for %u bytes\n",  pics710Driver->bytesToRead);
		return -1;
	}
	memset (pics710Driver->pAcqData, 0, pics710Driver->bytesToRead);

	/*allocate DMA memory and clear it*/
	pics710Driver->bytesToRead = (pics710Driver->bufLength + 1) * 8;  /* bytes to be read from ics-710 memory*/
	pics710Driver->pAcqData = (long *) ics710AllocateDmaBuffer (pics710Driver->hDevice, pics710Driver->bytesToRead);
	if (NULL == pics710Driver->pAcqData)
	{
		printf ("Error: Not enough memory for %u bytes\n",  pics710Driver->bytesToRead);
		return -1;
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
		printf("can't reset buffer register(ics710DrvLongout.cpp), errorCode: %d \n", errorCode);
		return errorCode;
	}

	printf("reconfigure totoalChannel to %d channels \n", pics710Driver->totalChannel);
	return 0;
}

static int setSamples(ics710Driver *pics710Driver, int val)
{
	int errorCode;

	if (val == pics710Driver->nSamples) return 0;

	pics710Driver->nSamples = val;
	pics710Driver->acqLength = pics710Driver->nSamples - 1;
	if (pics710Driver->control.packed_data == 0)
		pics710Driver->bufLength = (pics710Driver->totalChannel * pics710Driver->nSamples / 2) - 1;
	else
		pics710Driver->bufLength = (pics710Driver->totalChannel * pics710Driver->nSamples / 4) - 1;
	if (pics710Driver->bufLength > 262143)
	{
		printf ("error(ics710DrvLongout.cpp): Calculated buf_len > 256K(4 MBytes memory board), set to Max. 262143 \n");
		pics710Driver->bufLength = 262143;
	}

	/*free the previous DMA memory and then re-allocate it*/
	ics710FreeDmaBuffer (pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead);
	pics710Driver->bytesToRead = (pics710Driver->bufLength + 1) * 8;  /* bytes to be read from ics-710 memory*/
	pics710Driver->pAcqData = (long *) ics710AllocateDmaBuffer (pics710Driver->hDevice, pics710Driver->bytesToRead);
	if (NULL == pics710Driver->pAcqData)
	{
		printf ("error(ics710DrvLongout.cpp): Not enough memory for %u bytes\n",  pics710Driver->bytesToRead);
		return -1;
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
		printf("can't reset buffer register(ics710DrvLongout.cpp), errorCode: %d \n", errorCode);
		return errorCode;
	}

	printf("reconfigure samples/channel to %d samples \n", pics710Driver->nSamples);

	return 0;
}

static int setSamplingRate(ics710Driver *pics710Driver, int val)
{
	int errorCode;

	if (abs((1000*pics710Driver->ics710AdcClockRate/(256.0/(1<<pics710Driver->control.oversamp_ratio)) -val)) < 1.00) return 0;

	pics710Driver->ics710AdcClockRate = val * (256.0/(1<<pics710Driver->control.oversamp_ratio))/1000.0;
	if (pics710Driver->ics710AdcClockRate < 0.256)
	{
		printf("error: sampling rate is too low, set to the lowest speed 1KHz \n");
		pics710Driver->ics710AdcClockRate = 0.256;
	}
	if (pics710Driver->ics710AdcClockRate > 13.824)
	{
		printf("error: sampling rate is too high, set to the highest speed 216KHz \n");
		pics710Driver->ics710AdcClockRate = 13.824;
	}

	if (ICS710_OK != (errorCode = ics710ADCClockSet (pics710Driver->hDevice, &pics710Driver->ics710AdcClockRate, &pics710Driver->actualADCRate)))
	{
		printf("can't reconfigure ADC clock in ics710DrvLongout.cpp/setSamplingRate, errorCode: %d \n", errorCode);
		return errorCode;
	}
	printf("reconfigure data output rate by setSamplingRate: %3.3f KHz \n", 1000*pics710Driver->ics710AdcClockRate/(256.0/(1<<pics710Driver->control.oversamp_ratio)));

	return 0;
}

typedef int (*ics710LongoutFunc)(ics710Driver *pics710Driver, int val);
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
	  ics710Debug("ics710InitLongoutRecord: record name: %s, card: %d, link name: %s \n", plongout->name, pics710RecPrivate->card, pics710RecPrivate->name);

	  for (i = 0; i < MAX_LONGOUT_FUNC; i++)
	  {
	       if (0 == strcmp(pics710RecPrivate->name, parseLongoutString[i].name))
	       {
		    	ics710LongoutFuncStruct* pics710LongoutFuncStruct = new ics710LongoutFuncStruct;
		    	pics710LongoutFuncStruct->wfunc = parseLongoutString[i].wfunc;
		        pics710RecPrivate->pvt = pics710LongoutFuncStruct;
		        ics710Debug("parseLongoutString[i].name: %s \n", parseLongoutString[i].name);
		        return 0;
	       }
	  }

	  return -1;
}

template<> int ics710WriteRecordSpecialized(longoutRecord* plongout)
{
	  int status;
	  int val;

	  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(plongout->dpvt);
	  ics710Driver* pics710Driver = &ics710Drivers[pics710RecPrivate->card];
	  ics710LongoutFuncStruct* pics710LongoutFuncStruct = reinterpret_cast<ics710LongoutFuncStruct*>(pics710RecPrivate->pvt);

	  val = plongout->val;
	  status = pics710LongoutFuncStruct->wfunc(pics710Driver, val);

	  return status;
}
