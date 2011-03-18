/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on Mar-03-2011
 * */
#include "ics710Drv.h"
#include "ics710Dev.h"

#include <dbScan.h>
#include <dbAccess.h>
#include <waveformRecord.h>
#include "epicsTime.h"

#include <string.h>
#include <stdio.h>

#define MAX_WF_FUNC 2
/*global variable: defined in ics710DrvInit.cpp*/
extern  ics710Driver ics710Drivers[MAX_DEV];
//epicsTimeStamp endTime;

static int getRawVolt(void* dst, const double* src, unsigned effectiveSamples)
//	  pics710WfFuncStruct->rfunc(pwf->bptr, buffer, nsamples);
{
	  memcpy((double *) dst, src, effectiveSamples * sizeof(double));
	  return 0;
}

static int getAveVolt(void* dst, const double* src, unsigned effectiveSamples)
//	  pics710WfFuncStruct->rfunc(pwf->bptr, buffer, nsamples);
{
	  unsigned i = 0;
	  double* dDst = static_cast<double*>(dst);
	  //Mar-11-2011: must reset dDst[0] to 0;
	  dDst[0] = 0.0;

	  for (i= 0; i < effectiveSamples; i++)
	  {
		  dDst[0] += src[i];
		  //printf("src[%d] = %f	", i, src[i]);
	  }
	  dDst[0] /= effectiveSamples;
	  //printf("effectiveSamples: %d, averaged value: %f \n", effectiveSamples, dDst[0]);

	  return 0;
}

typedef int (*ics710WfFunc)(void* dst, const double* src, unsigned effectiveSamples);

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
  {"WRAW", getRawVolt},
  {"WAVE", getAveVolt},
  //{"WRMS", getRmsVolt},
};

template<> int ics710InitRecordSpecialized(waveformRecord* pwf)
{
	unsigned i;
	  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(pwf->dpvt);
	  ics710Debug("ics710InitWfRecord: card: %d, channel: %d, name: %s \n", pics710RecPrivate->card, pics710RecPrivate->channel, pics710RecPrivate->name);

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
	char buf[30];
	  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(pwf->dpvt);
	  ics710Driver* pics710Driver = &ics710Drivers[pics710RecPrivate->card];
	  ics710WfFuncStruct* pics710WfFuncStruct = reinterpret_cast<ics710WfFuncStruct*>(pics710RecPrivate->pvt);
	  ics710Debug("channel #%d: waveform record (%s) read started \n ", pics710RecPrivate->channel, pwf->name);

	  if ((0 == strcmp(pics710RecPrivate->name, "WVOL")) && (pics710Driver->nSamples > pwf->nelm))
	  {
		  pics710Driver->nSamples = pwf->nelm;
		  pics710Driver->truncated++;
		  printf("Warning: set 'NELM' field > nSamples \n ");
	  }

	  //epicsMutexLock(pics710Driver->daqMutex);
	  /*discard the garbage data(1024/totalChannel) at the beginning of the waveform
	   * 03/04/2011:don't need to discard any data since the garbage data only occur at the first acquisition
	   * 03/10/2011: still get fake data if totalChannel > 2
	   * */
	  pics710WfFuncStruct->rfunc(pwf->bptr, (const double*) &pics710Driver->chData[pics710RecPrivate->channel][1024/pics710Driver->totalChannel],
			  (pics710Driver->nSamples - 1024/pics710Driver->totalChannel));
	  //pics710WfFuncStruct->rfunc(pwf->bptr, (const double*) &pics710Driver->chData[pics710RecPrivate->channel][0],pics710Driver->nSamples );
	  //epicsMutexUnlock(pics710Driver->daqMutex);

	  pwf->nord = pics710Driver->nSamples;
	  ics710Debug("channel #%d: waveform record (%s) read completed \n\n", pics710RecPrivate->channel, pwf->name);
/*
	  epicsTimeGetCurrent(&endTime);
	  epicsTimeToStrftime(buf, 30, "%Y/%m/%d %H:%M:%S.%06f", &endTime);
	  printf("endTime: %s \n",buf);
*/
	  return 0;
}

/*It seems ics710GetioscanpvtSpecialized is called only once during initialization:
 * enable debug, only see ics710Debug called once
 * */
template<> IOSCANPVT ics710GetioscanpvtSpecialized(waveformRecord* pwf)
{
  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(pwf->dpvt);
  ics710Driver* pics710Driver = &ics710Drivers[pics710RecPrivate->card];
  ics710Debug("card #%d: waveform record (%s) Intr I/O occurred \n ", pics710RecPrivate->card, pwf->name);
  return pics710Driver->ioscanpvt;
}

