/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on Mar-03-2011
 * */
/*ics710Asbu.c: calculate the time spent on raw data readout(read())and the whole loop, then put them into ai records;
 * put triggerRate to ai record, get dcOffset of individual channel from ao record;
 * */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "registryFunction.h"
#include "aSubRecord.h"
#include "epicsExport.h"
#include "ics710Drv.h"

int ics710AsubDebug = 0;

/*global variable: defined in ics710DrvInit.cpp*/
extern double timeAfterADCInt;
extern double triggerRate;
extern double timeAfterRead;
extern double dcOffset[MAX_CHANNEL];

typedef long (*processMethod)(aSubRecord *precord);

static long ics710AsubInit(aSubRecord *precord,processMethod process)
{
    if (ics710AsubDebug)
        printf("Record %s called ics710AsubInit(%p, %p), initial value is: %d\n",
               precord->name, (void*) precord, (void*) process, precord->val);
    return(0);
}

static long ics710AsubProcess(aSubRecord *precord)
{
    double temp;
    //char buf[30];
    epicsTimeStamp now;
    double timeAtAsub;
    double rwDataReadTime;
    double loopTime;
    int i = 0;
    int channel = 0;

    if (ics710AsubDebug)
    {
    	//memcpy(&temp, (&precord->a)[0], sizeof(double));//works
    	memcpy(&temp, (double *)precord->a, sizeof(double));//works
    	printf("Record %s called and INPA value is: %f \n",precord->name, temp);//works
    	//printf("Record %s called and INPA value is: %f \n",precord->name,  * (double *)(precord->a));work
    	//printf("Record %s called and INPA value is: %f \n",precord->name, (&precord->a)[0]);//doesn't work
    }
    //printf("INPA is: %s \n", precord->inpa.text);//doesn't work: get NULL
    //printf("INPA is: %s \n", precord->inpa);// get the input link string, but can't use the string
    for (i=0; i<sizeof(precord->name); i++)
    {
    	if (isdigit(precord->name[i]))
    	{
    		channel = precord->name[i]-0x30;
    	}
    }
    dcOffset[channel]= *(double *)precord->a;
	//printf("channel-%d DC offset: %f \n",channel,dcOffset[channel]);

    epicsTimeGetCurrent(&now);
    timeAtAsub = now.secPastEpoch + now.nsec/1000000000.0;

    rwDataReadTime = 1000 * (timeAfterRead - timeAfterADCInt);//ms
    loopTime = 1000 * (timeAtAsub - timeAfterADCInt);

    memcpy((&precord->vala)[0], &triggerRate, sizeof(double));
    memcpy((&precord->vala)[1], &rwDataReadTime, sizeof(double));
    //memcpy((&precord->vala)[2], &loopTime, sizeof(double));
    memcpy(precord->valc, &loopTime, sizeof(double));//works

	return(0);
}

/* Register these symbols for use by IOC code: */
//epicsExportAddress(int, ics710AsubDebug);
epicsRegisterFunction(ics710AsubInit);
epicsRegisterFunction(ics710AsubProcess);
