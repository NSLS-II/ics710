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

#include "ics710Drv.h"
#include "ics710Dev.h" /*struct ics710RecPrivate*/

int ics710ProcessWfAsubDebug = 0;

/*global variable: defined in ics710DrvInit.cpp*/
extern ics710Driver ics710Drivers[MAX_DEV];

typedef long (*processMethod)(aSubRecord *precord);

static long ics710ProcessWfAsubInit(aSubRecord *precord,processMethod process)
{
    if (ics710ProcessWfAsubDebug)
        printf("Record %s called ics710ProcessWfAsubInit(%p, %p), initial value is: %d\n",
               precord->name, (void*) precord, (void*) process, precord->val);
    return(0);
}

static long ics710ProcessWfAsubProcess(aSubRecord *precord)
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
static long ics710ProcessCirBuffer(aSubRecord *precord)
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

/* Register these symbols for use by IOC code: */
//epicsExportAddress(int, ics710AsubDebug);
epicsRegisterFunction(ics710ProcessWfAsubInit);
epicsRegisterFunction(ics710ProcessWfAsubProcess);
epicsRegisterFunction(ics710ProcessCirBuffer);

