/*Yong Hu: 02-08-2010*/
#include "ics710Drv.h"
#include "ics710Dev.h"

#include <dbScan.h>
#include <dbAccess.h>
#include <waveformRecord.h>

#include <string.h>
#include <stdio.h>

#define MAX_WF_FUNC 1

static int computedVolt(void* dst, const double** src, unsigned nSamples)
//	  pics710WfFuncStruct->rfunc(pwf->bptr, buffer, nsamples);
{
	  unsigned i = 0;
	  double* dDst = static_cast<double*>(dst);

	  for (i= 0; i < nSamples; i++)
	  {
		  dDst[i] = *src[i];
	  }
	  ics710Debug("computeVolt: simple \n ");
	  return 0;
}

typedef int (*ics710WfFunc)(void* dst, const double** src, unsigned nSamples);

struct ics710WfFuncStruct
{
	  ics710WfFunc rfunc;
};

static struct
{
  const char* name;
  ics710WfFunc rfunc;
} parseWfString[MAX_WF_FUNC] =
{
  {"WVOL", computedVolt},
  //{"WAVE", averageVolt},
  //{"WRMS", rmsVolt},
};

template<> int ics710InitRecordSpecialized(waveformRecord* pwf)
{
	unsigned i;
	  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(pwf->dpvt);
	  ics710Debug("ics710InitRecord: card: %d, channel: %d, name: %s \n", pics710RecPrivate->card, pics710RecPrivate->channel, pics710RecPrivate->name);

	  for (i = 0; i < MAX_WF_FUNC; i++)
	  {
	       if (0 == strcmp(pics710RecPrivate->name, parseWfString[i].name))
	       {
		    	ics710WfFuncStruct* pics710WfFuncStruct = new ics710WfFuncStruct;
		    	pics710WfFuncStruct->rfunc = parseWfString[i].rfunc;
		        pics710RecPrivate->pvt = pics710WfFuncStruct;
		        ics710Debug("parseWfString[i].name: %s \n", parseWfString[i].name);
		        return 0;
	       }
	  }

	  return -1;
}

template<> int ics710ReadRecordSpecialized(waveformRecord* pwf)
{
	  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(pwf->dpvt);
	  ics710Driver* pics710Driver = &ics710Drivers[pics710RecPrivate->card];
	  ics710WfFuncStruct* pics710WfFuncStruct = reinterpret_cast<ics710WfFuncStruct*>(pics710RecPrivate->pvt);
	  //const double** buffer = &pics710Driver->chData[pics710RecPrivate->channel];
	  ics710Debug("channel #%d: waveform record read started \n ", pics710RecPrivate->channel);

	  if (pics710Driver->nSamples > pwf->nelm)
	  {
		  pics710Driver->nSamples = pwf->nelm;
		  pics710Driver->truncated++;
		  printf("Warning: set 'NELM' field > nSamples \n ");
	  }

	  epicsMutexLock(pics710Driver->daqMutex);
	  //ics710Debug("channel #%d: start to copy data to waveform buffer \n", pics710RecPrivate->channel);
	  //pics710WfFuncStruct->rfunc(pwf->bptr, buffer, pics710Driver->nSamples);
	  /*discard the garbage data(1024/totalChannel) at the beginning of the waveform*/
	  memcpy((double*) pwf->bptr, (const double*) &pics710Driver->chData[pics710RecPrivate->channel][1024/pics710Driver->totalChannel],
			  (pics710Driver->nSamples - 1024/pics710Driver->totalChannel) * sizeof(double));//works
	  //memcpy((double*) pwf->bptr, pics710Driver->chData[pics710RecPrivate->channel], pics710Driver->nSamples * sizeof(double));//works
	  //ics710Debug("channel #%d: copy buffer done \n", pics710RecPrivate->channel);
	  epicsMutexUnlock(pics710Driver->daqMutex);

	  pwf->nord = pics710Driver->nSamples;
	  ics710Debug("channel #%d: waveform record read completed \n\n", pics710RecPrivate->channel);

	  return 0;
}


template<> IOSCANPVT ics710GetioscanpvtSpecialized(waveformRecord* pwf)
{
  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(pwf->dpvt);
  ics710Driver* pics710Driver = &ics710Drivers[pics710RecPrivate->card];
  ics710Debug("card #%d: Intr I/O occurred \n ", pics710RecPrivate->card);
  return pics710Driver->ioscanpvt;
}

