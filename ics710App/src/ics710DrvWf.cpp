/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on Mar-03-2011
 * */
/*ics710DrvWf.cpp: used by ics710DevWf.cpp, put the acquired data of individual channel to waveform record*/

#include "ics710Drv.h"
#include "ics710Dev.h"

#include <dbScan.h>
#include <dbAccess.h>
#include <waveformRecord.h>
#include "epicsTime.h"

#include <string.h>
#include <stdio.h>

int maxChannel = 0;
#define MAX_WF_FUNC 2
/*global variable: defined in ics710DrvInit.cpp*/
extern  ics710Driver ics710Drivers[MAX_DEV];

//static int getRawVolt(void* dst, const double* src, unsigned effectiveSamples)
static int getRawData(void* dst, const int* src, unsigned effectiveSamples)
{
	  //((double *) dst, src, effectiveSamples * sizeof(double));
	  memcpy((int *) dst, src, effectiveSamples * sizeof(int));
	  return 0;
}

//static int getAveVolt(void* dst, const double* src, unsigned effectiveSamples)
static int getVolts(void* dst, const int* src, unsigned effectiveSamples)
{
	  unsigned i = 0;
	  double* dDst = static_cast<double*>(dst);

	  for (i= 0; i < effectiveSamples; i++)
	  {
		  //dDst[0] = (src[i]/(256* 8388608.0)) * (10.00 / (1+pics710Driver->gainControl.input_voltage_range)) - dcOffset[channel];;
		  //pics710Driver->chData[channel][nSamples] = pics710Driver->rawData[channel][nSamples]/ (256* 8388608.0) * (10.00 / (1+pics710Driver->gainControl.input_voltage_range)) - dcOffset[channel];
		  //printf("src[%d] = %f	", i, src[i]);
	  }

	  return 0;
}

/* choose the function according to LINK string name */
typedef int (*ics710WfFunc)(void* dst, const int* src, unsigned effectiveSamples);
struct ics710WfFuncStruct
{
	  ics710WfFunc rfunc; /* read function*/
};

static struct
{
  const char* name;
  ics710WfFunc rfunc;
} parseWfString[MAX_WF_FUNC] =
{
  {"WRAW", getRawData},/*raw waveform: integer data*/
  {"WVOL", getVolts},/*voltage waveform*/
  //{"WAVE", getAveVolt},/*averaged waveform*/
  //{"WRMS", getRmsVolt},
};

/* associate the link string 'name' with function */
template<> int ics710InitRecordSpecialized(waveformRecord* pwf)
{
	unsigned i;
	  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(pwf->dpvt); /*retrieve record private data*/
	  ics710Debug("ics710InitWfRecord: card: %d, channel: %d, name: %s \n", pics710RecPrivate->card, pics710RecPrivate->channel, pics710RecPrivate->name);
	  if (pics710RecPrivate->channel > maxChannel) maxChannel = pics710RecPrivate->channel;

	  for (i = 0; i < MAX_WF_FUNC; i++)
	  {
	       if (0 == strcmp(pics710RecPrivate->name, parseWfString[i].name))
	       {
		    	ics710WfFuncStruct* pics710WfFuncStruct = new ics710WfFuncStruct;
		    	pics710WfFuncStruct->rfunc = parseWfString[i].rfunc;
		        pics710RecPrivate->pvt = pics710WfFuncStruct; /* save the function to pvt */
		        ics710Debug("parseWfString[i].name: %s \n", parseWfString[i].name);
		        return 0;
	       }
	  }

	  printf("unable to initialize waveform record %s \n", pwf->name);
	  return -1;
}

/* put the acquired data of individual channel to waveform record: one waveform record for one channel */
template<> int ics710ReadRecordSpecialized(waveformRecord* pwf)
{
	//char buf[30];
	unsigned int nSamples = 0;
	unsigned i = 0;
	  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(pwf->dpvt);/*retrieve record private data*/
	  ics710Driver* pics710Driver = &ics710Drivers[pics710RecPrivate->card];/*retrieve ics710 IOC data*/
	  ics710WfFuncStruct* pics710WfFuncStruct = reinterpret_cast<ics710WfFuncStruct*>(pics710RecPrivate->pvt);/*retrieve the function*/
	  ics710Debug("channel #%d: waveform record (%s) read started \n ", pics710RecPrivate->channel, pwf->name);

/*	  //if ((0 == strcmp(pics710RecPrivate->name, "WRAW")) && (pics710Driver->nSamples  != pwf->nelm))
	  if ((0 == strcmp(pics710RecPrivate->name, "WRAW")) && (pics710Driver->nSamples  > pwf->nelm))
	  {
		  //printf("Error:NELM(%d in ics710Channel.substitions) is not equal to nSamples(%d in st.cmd): modify ics710Channel.substitions or st.cmd to make them match\n", pwf->nelm, pics710Driver->nSamples);
		  //pwf->nelm = pics710Driver->nSamples;
		  printf("Error:NELM(%d in ics710Channel.substitions) is less than to nSamples(%d in st.cmd): modify ics710Channel.substitions or st.cmd to make them match\n", pwf->nelm, pics710Driver->nSamples);
		  return -1;
		 /// pics710Driver->truncated++;
	  }
*/
	  /*2011-04-29:for better display of waveform data using BOY, set NELM equal to samples/ch */
	  pwf->nelm = pics710Driver->nSamples;

	  if (pics710Driver->totalChannel != (maxChannel + 1))
	  {
		  //printf("maxChannel: %d \n", maxChannel + 1);
		  printf("Error:max. CHANNEL(%d in ics710Channel.substitions) is not equal to totalChannel(%d in st.cmd): modify ics710Channel.substitions or st.cmd to make them match\n",(maxChannel + 1), pics710Driver->totalChannel);
		  return -1;
	  }

	  /* Trick: get rid of the garbage data: set them as the correct one; 1024/totalChannel samples at the beginning, garbage samples occur again every 32*1024/totalChannel */
/*	  for (nSamples = 0; nSamples < pics710Driver->nSamples; nSamples++)
	  {
		  if (0 == (nSamples % ((32*1024)/pics710Driver->totalChannel))) //32K = 32*102
		  {
			  //printf("nSamples at the beginning: %d \n",nSamples);
			  //for (i = 1; i < 1024/pics710Driver->totalChannel; i++)
			  for (i = 0; i < 1024/pics710Driver->totalChannel; i++)
			  //for (i = 1; i < 1024; i++)
			  {
				  //if ((nSamples + i) > pics710Driver->nSamples) break;
				  //pics710Driver->chData[pics710RecPrivate->channel][nSamples + i] = pics710Driver->chData[pics710RecPrivate->channel][nSamples];
				  if ((nSamples + i + 1024/pics710Driver->totalChannel) > pics710Driver->nSamples)
					  pics710Driver->chData[pics710RecPrivate->channel][nSamples + i] = pics710Driver->chData[pics710RecPrivate->channel][pics710Driver->nSamples];
				  else
					  pics710Driver->chData[pics710RecPrivate->channel][nSamples + i] = pics710Driver->chData[pics710RecPrivate->channel][nSamples + i + 1024/pics710Driver->totalChannel];
			  }
			  nSamples = nSamples + i;
			  //printf("nSamples end: %d; i: %d \n",nSamples, i);
		  }
	  }
*/
	  /*discard the garbage data(1024/totalChannel) at the beginning of the waveform
	   * 03/04/2011:don't need to discard any data since the garbage data only occur at the first acquisition
	   * 03/10/2011: still get fake data if totalChannel > 2
	   * 04/02/2011: play tricks on the garbage data
	   * July/12/2011: new boards don't have data glitch issue*/
	  epicsMutexLock(pics710Driver->daqMutex);
	  //pics710WfFuncStruct->rfunc(pwf->bptr, (const double*) &pics710Driver->chData[pics710RecPrivate->channel][1024/pics710Driver->totalChannel], (pics710Driver->nSamples - 1024/pics710Driver->totalChannel));
	  //pics710WfFuncStruct->rfunc(pwf->bptr, (const int*) &pics710Driver->rawData[pics710RecPrivate->channel][0], pics710Driver->nSamples);
	  if (0 == strcmp(pics710RecPrivate->name, "WRAW"))
	  {
		  memcpy((int *)pwf->bptr, (const int *) &pics710Driver->rawData[pics710RecPrivate->channel][0], sizeof(int) * pics710Driver->nSamples);
	  }
	  else if (0 == strcmp(pics710RecPrivate->name, "WVOL"))
	  {
		  memcpy((double *)pwf->bptr, (const double*) &pics710Driver->chData[pics710RecPrivate->channel][0], sizeof(double) * pics710Driver->nSamples);
	  }
	  epicsMutexUnlock(pics710Driver->daqMutex);

	  pwf->nord = pics710Driver->nSamples; /*waveform read-out is completed*/

	  ics710Debug("channel #%d: waveform record (%s) read completed \n\n", pics710RecPrivate->channel, pwf->name);
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

