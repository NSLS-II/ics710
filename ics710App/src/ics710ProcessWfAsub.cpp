/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on Mar-03-2011
 * */
/*ics710ProcessWfAsbu.c: process waveform record: mean, min, max, rms, integral, std, etc.; */

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

int ics710ProcessWfAsubDebug = 0;

/*global variable: defined in ics710DrvInit.cpp*/
extern ics710Driver ics710Drivers[MAX_DEV];
//extern double timeAfterADCInt;
//extern double triggerRate;
//extern double timeAfterRead;
//extern double dcOffset[MAX_CHANNEL];

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
    //DBADDR *paddr;
    double ave[MAX_CHANNEL]  = {0.0, 0.0, 0.0, 0.0};
    double max[MAX_CHANNEL]  = {0.0, 0.0, 0.0, 0.0};
    double min[MAX_CHANNEL]  = {0.0, 0.0, 0.0, 0.0};
    double sum[MAX_CHANNEL]  = {0.0, 0.0, 0.0, 0.0};

    /*copy the waveform data*/
	memcpy(&temp, (double *)precord->a, precord->noa * sizeof(precord->fta));
    if (ics710ProcessWfAsubDebug)
    	printf("Record %s called and INPA value is: #100: %f #1000: %f\n",precord->name, temp[100], temp[1000]);//works

    /* get #card and #channel information from aSub record name */
    if (pch = strstr (precord->name,"Card"))
    {
    	card = *(pch + 4) - 0x30;
    	//printf ("card #: %d \n",card);
    }
    if (pch = strstr (precord->name,"Ch"))
    {
    	channel = *(pch + 2) - 0x30;
    	//printf ("channel #: %d \n",channel);
    }
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

    /* put the calculated results into ai records*/
    *(double *)precord->vala = ave[channel];
    *(double *)precord->valb = max[channel];
    *(double *)precord->valc = min[channel];
    *(double *)precord->vald = sum[channel];
    //printf("channel #%d: #0: %f; #16001:%f; mean: %f; max: %f, min: %f, sum: %f \n",channel, pics710Driver->chData[channel][0], pics710Driver->chData[channel][16001], ave[channel], max[channel], min[channel], sum[channel]);

	return(0);
}

/* Register these symbols for use by IOC code: */
//epicsExportAddress(int, ics710AsubDebug);
epicsRegisterFunction(ics710ProcessWfAsubInit);
epicsRegisterFunction(ics710ProcessWfAsubProcess);
