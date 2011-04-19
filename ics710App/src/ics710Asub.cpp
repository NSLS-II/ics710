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
#include "link.h"
#include "dbAddr.h"
#include "dbCommon.h" /* precord: now = paddr->precord->time;*/
#include "epicsTime.h"

#include "ics710Drv.h"
#include "ics710Dev.h" /*struct ics710RecPrivate*/

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
    char buf[30];
    epicsTimeStamp now;
    double timeAtAsub;
    double rwDataReadTime;
    double loopTime;
    int i = 0;
    int channel = 0;
    DBADDR *paddr;
    ics710RecPrivate *pics710RecPrivate;

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
/*
 // doesn't work for channels > 9
    for (i=0; i<sizeof(precord->name); i++)
    {
    	if (isdigit(precord->name[i]))
    	{
    		channel = precord->name[i]-0x30;
    	}
    }
 */
    struct link *plink = &precord->inpb;
    if (plink->type != DB_LINK) return -1;
    paddr = (DBADDR *)plink->value.pv_link.pvt; /*get the pvt/address of the record*/
    pics710RecPrivate = (ics710RecPrivate*)(paddr->precord->dpvt);/*retrieve the waveform record private data*/
    channel = pics710RecPrivate->channel;
    dcOffset[channel]= *(double *)precord->a;
    //printf("change the DC offset of ch-%d to %f Volts \n",channel, *(double *)precord->a);

    /* get time-stamp of another record: refer to recGblGetTimeStamp(prec) -> dbGetTimeStamp(plink, &prec->time) */
    /*
    //struct link *plink = &prec->tsel;
    struct link *plink = &precord->outa;
    if (plink->type != DB_LINK)
        return -1;
    paddr = (DBADDR *)plink->value.pv_link.pvt;
    now = paddr->precord->time;
    printf("OUTA %s time-stamp is: ", paddr->precord->name);
    //time.strftime(buf,30,"%Y/%m/%d %H:%M:%S.%06f");
    epicsTimeToStrftime(buf, 30, "%Y/%m/%d %H:%M:%S.%06f", &now);
    printf("%s \n", buf);
    //epicsTimeShow(&now, 1);
*/
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
