/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on Mar-03-2011
 * */
/*ics710ProcessWfAsbu.cpp: process waveform record: mean, min, max, rms, integral, std, etc.; */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "registryFunction.h"
#include "aSubRecord.h"
#include "epicsExport.h"
#include "link.h"
#include "dbAddr.h"
#include "dbCommon.h" /* precord: now = paddr->precord->time;*/
#include "epicsTime.h"
#include "waveformRecord.h"

#include "ics710Drv.h"
#include "ics710Dev.h" /*struct ics710RecPrivate*/

static int ics710ProcessWfAsubDebug = 0;
static bool timeAxisAsubInitialized = FALSE;
static double *ptimeAxis;

/*global variable: defined in ics710DrvInit.cpp*/
extern ics710Driver ics710Drivers[MAX_DEV];
extern double timeAfterADCInt;
extern double triggerRate;
extern double timeAfterRead;
extern double dcOffset[MAX_CHANNEL];
extern double inputRange[MAX_CHANNEL];

typedef long (*processMethod)(aSubRecord *precord);

static long ics710InitWfAsub(aSubRecord *precord,processMethod process)
{
    if (ics710ProcessWfAsubDebug)
        printf("Record %s called ics710ProcessWfAsubInit(%p, %p), initial value is: %d\n",
               precord->name, (void*) precord, (void*) process, precord->val);
    return(0);
}

static long ics710ProcessWfAsub(aSubRecord *precord)
{
    double temp[MAX_SAMPLE];
    //char buf[30];
    //epicsTimeStamp now;
    unsigned i = 0;
    char * pch;
    int card = 0;
    int channel = 0;
    DBADDR *paddr;
    ics710RecPrivate *pics710RecPrivate;
    double ave[MAX_CHANNEL]  = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    double max[MAX_CHANNEL]  = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    double min[MAX_CHANNEL]  = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    double sum[MAX_CHANNEL]  = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    double std[MAX_CHANNEL]  = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
//get CARD and CHANNEL info from INPA --waveform record
    struct link *plink = &precord->inpa;
    if (DB_LINK != plink->type) return -1;
    paddr = (DBADDR *)plink->value.pv_link.pvt;
    pics710RecPrivate = (ics710RecPrivate *)paddr->precord->dpvt;
    card =  pics710RecPrivate->card;
    channel = pics710RecPrivate->channel;
	//printf("Record %s called and INPA value is: #100: %f #1000: %f\n",precord->name, temp[100], temp[1000]);//works

	ics710Driver* pics710Driver = &ics710Drivers[card];/*retrieve ics710 IOC data*/

    /*calculate max, min, sum, mean*/
    max[channel] = pics710Driver->chData[channel][0];
    min[channel] = pics710Driver->chData[channel][0];
    for (i= 0; i < pics710Driver->nSamples; i++)
    {
    	sum[channel] += pics710Driver->chData[channel][i];

    	if (pics710Driver->chData[channel][i] > max[channel])
    		max[channel] = pics710Driver->chData[channel][i];

    	if (pics710Driver->chData[channel][i] < min[channel])
    		min[channel] = pics710Driver->chData[channel][i];
    }
    ave[channel] = sum[channel] / pics710Driver->nSamples;

    /*calculate standard deviation (RMS noise, not RMS value: Delta = sqrt[(Xi-Xmean)^2)]*/
    for(i = 0; i < pics710Driver->nSamples; i++)
    {
    	std[channel] += (pics710Driver->chData[channel][i] - ave[channel]) * (pics710Driver->chData[channel][i] - ave[channel]);
    }
    std[channel] = sqrt(std[channel] / pics710Driver->nSamples);

    /* put the calculated results into ai records*/
    *(double *)precord->vala = ave[channel];
    *(double *)precord->valb = max[channel];
    *(double *)precord->valc = min[channel];
    *(double *)precord->vald = sum[channel];
    *(double *)precord->vale = std[channel];
    //printf("channel #%d: #0: %f; #16001:%f; mean: %f; max: %f, min: %f, sum: %f \n",channel, pics710Driver->chData[channel][0], pics710Driver->chData[channel][16001], ave[channel], max[channel], min[channel], sum[channel]);

	return(0);
}

/*RMS noise over 60 samples (1-minute for 1Hz)*/
static long ics710ProcessCirBufferAsub(aSubRecord *precord)
{
    double temp[60];
    int i = 0;
    double sum = 0.0;
    double ave = 0.0;
    double rmsNoise = 0.0;
    /*copy the circular buffer */
    /*May-13-2011:[sizeof(precord->fta) == 2] != [4 == sizeof(double)]; must use sizeof(double)*/
	//memcpy(&temp, (double *)precord->a, precord->noa * sizeof(precord->fta));
	memcpy(&temp, (double *)precord->a, precord->noa * sizeof(double));
    //printf("noA: %d, size(fta): %d \n", precord->noa, sizeof(precord->fta));
	//memcpy(&temp, (&precord->a)[0], precord->noa * sizeof(double));//This also works
	if (ics710ProcessWfAsubDebug)
	{
		for (i = 0; i < 60; i++) printf("temp[%d] is: %f		",i, temp[i]);
		printf("\n");
	}

	for(i = 0; i < 60; i++)
		sum += temp[i];
	ave = sum / 60.0;
	//printf("sum: %f, ave is %f \n",sum, ave);

	for(i = 0; i < 60; i++)
		rmsNoise += (temp[i] - ave) * (temp[i] - ave);
	rmsNoise = sqrt(rmsNoise / 60.0);
	//printf("rmsNoise is %f \n",rmsNoise);
    *(double *)precord->vala = rmsNoise;
	return(0);
}

static long ics710ProcessMiscAsub(aSubRecord *precord)
{
    double temp;
    char buf[30];
    epicsTimeStamp now;
    double timeAtAsub;
    double rwDataReadTime;
    double loopTime;
    int i = 0;
    int channel = 0;
    DBADDR *paddr;
    ics710RecPrivate *pics710RecPrivate;

    if (ics710ProcessWfAsubDebug)
    {
    	//memcpy(&temp, (&precord->a)[0], sizeof(double));//works
    	memcpy(&temp, (double *)precord->a, sizeof(double));//works
    	printf("Record %s called and INPA value is: %f \n",precord->name, temp);//works
    	//printf("Record %s called and INPA value is: %f \n",precord->name,  * (double *)(precord->a));work
    	//printf("Record %s called and INPA value is: %f \n",precord->name, (&precord->a)[0]);//doesn't work
    }
    //printf("INPA is: %s \n", precord->inpa.text);//doesn't work: get NULL
    //printf("INPA is: %s \n", precord->inpa);// get the input link string, but can't use the string
//get channel info from waveform record
    struct link *plink = &precord->inpa;
    if (plink->type != DB_LINK) return -1;
    paddr = (DBADDR *)plink->value.pv_link.pvt; /*get the pvt/address of the record*/
    pics710RecPrivate = (ics710RecPrivate*)(paddr->precord->dpvt);/*retrieve the waveform record private data*/
    channel = pics710RecPrivate->channel;
 //input links: offset and input range which are used in ics710Daq.cpp
    dcOffset[channel]= *(double *)precord->b;
    inputRange[channel]= *(double *)precord->c;
    //printf("change the DC offset of ch-%d to %f Volts \n",channel, *(double *)precord->a);

    epicsTimeGetCurrent(&now);
    timeAtAsub = now.secPastEpoch + now.nsec/1000000000.0;

    rwDataReadTime = 1000 * (timeAfterRead - timeAfterADCInt);//ms
    loopTime = 1000 * (timeAtAsub - timeAfterADCInt);

 //triggerRate is computed in ics710Daq.cpp
    memcpy((&precord->vala)[0], &triggerRate, sizeof(double));
    memcpy((&precord->vala)[1], &rwDataReadTime, sizeof(double));
    //memcpy((&precord->vala)[2], &loopTime, sizeof(double));
    memcpy(precord->valc, &loopTime, sizeof(double));//works

	return(0);
}

static long ics710InitTimeAxisAsub(aSubRecord *precord,processMethod process)
{
	if (timeAxisAsubInitialized)
		return (0);
    if (NULL == (ptimeAxis = (double *)malloc(precord->nova * sizeof(double))))
    {
    	printf("out of memory: timeAxisAsubInit \n");
    	return -1;
    }
    else
    {
    	timeAxisAsubInitialized = TRUE;
    	//printf("allocate memory in timeAxisAsubInit successfully\n");
    }

    return(0);
}

static long ics710ProcessTimeAxisAsub(aSubRecord *precord)
{
	unsigned long nSample;
	double sampleLength;
	unsigned int i = 0;
	DBLINK *plink;
	DBADDR *paddr;
	waveformRecord *pwf;

//input links: number of samples(data points), sample length (N ms)
	memcpy(&nSample, (unsigned long *)precord->a, precord->noa * sizeof(unsigned long));
	memcpy(&sampleLength, (double *)precord->b, precord->nob * sizeof(double));

//using effective number of samples(samples/channel or 'NELM') in the waveform instead of 'NOA'(max. samples/ch) for data analysis
	plink = &precord->outa;
	if (DB_LINK != plink->type) return -1;
	//plink->value.pv_link.precord->name;
	//printf("This aSub record name is: %s, %s \n", precord->name, plink->value.pv_link.precord->name);
	paddr = (DBADDR *)plink->value.pv_link.pvt;
	pwf = (waveformRecord *)paddr->precord;
	pwf->nelm = nSample;
	//printf("number of effective samples(samples/ch): %d \n", pwf->nelm);

	for (i = 0; i < pwf->nelm; i++)
	{
		ptimeAxis[i] = i * (sampleLength/nSample);
	}
	//printf("ptimeAxis-1: %f;  ptimeAxis-max: %f;\n", ptimeAxis[1], ptimeAxis[pwf->nelm-1]);

//output links: waveform
	memcpy((double *)precord->vala, ptimeAxis, pwf->nelm * sizeof(double));
	//printf("put all output links values \n");

	return(0);
}

/* Register these symbols for use by IOC code: */
//epicsExportAddress(int, ics710AsubDebug);
epicsRegisterFunction(ics710InitWfAsub);
epicsRegisterFunction(ics710ProcessWfAsub);
epicsRegisterFunction(ics710ProcessCirBufferAsub);
epicsRegisterFunction(ics710ProcessMiscAsub);
epicsRegisterFunction(ics710InitTimeAxisAsub);
epicsRegisterFunction(ics710ProcessTimeAxisAsub);

