/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on Mar-03-2011
 * */
/* process waveform record: mean, min, max, rms, integral, std, etc.;
 * process circular buffer data: charge rate, etc;
 * construct time axis;
 * */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>

#include "registryFunction.h"
#include "aSubRecord.h"
#include "epicsExport.h"
#include "link.h"
#include "dbAddr.h"
#include "dbCommon.h"
#include "epicsTime.h"
#include "waveformRecord.h"
#include "longoutRecord.h"
#include "errlog.h"
#include "epicsTime.h"

#include "ics710Drv.h"
#include "ics710Dev.h" //struct ics710RecPrivat
//basic processing: max, min, sum/integral, mean, std; called by processWf()
void
processBasic(const double *pData, unsigned startPoint, unsigned endPoint,
             double *max, double *min, double *sum, double *ave, double *std)
{
    //*max = pData[0];
    //*min = pData[0];
    *max = pData[startPoint];
    *min = pData[startPoint];
    *sum = 0.0;
    *ave = 0.0;
    *std = 0.0;
    unsigned int i = 0;

    //base data processing: max, min, sum/integral, mean, std
    for (i = startPoint; i < endPoint; i++)
    {
        if (pData[i] > *max)
            *max = pData[i];
        if (pData[i] < *min)
            *min = pData[i];
        *sum += pData[i];
    }
    *ave = (*sum) / (endPoint - startPoint);
    //calculate standard deviation (RMS noise: Delta = sqrt[(Xi-Xmean)^2)]
    for (i = startPoint; i < endPoint; i++)
    {
        *std += (pData[i] - (*ave)) * (pData[i] - (*ave));
    }
    *std = sqrt((*std) / (endPoint - startPoint));

    return;
}

/* see ics710Channel.db for INP/OUT fields
 * data processing: convert raw integer data (32-bit) to voltages calibrated
 * against input range, range coefficient, offset, etc.
 * basic results: max, min, sum/integral, mean and std in the whole waveform
 * and in part of the waveform (ROI, region of interest)
 * */
static long
processWf(aSubRecord *precord)
{
    assert(precord != NULL);
    //get CARD info from INPA: waveform record for raw data
    struct link *plink = &precord->inpa;
    assert((plink != NULL) && (DB_LINK == plink->type));
    struct dbAddr *paddr = (DBADDR *) plink->value.pv_link.pvt;
    ics710RecPrivate *pics710RecPrivate =
            (ics710RecPrivate *) paddr->precord->dpvt;
    int card = pics710RecPrivate->card;
    //retrieve ics710 IOC data from global variable ics710Drivers[]
    ics710Driver* pics710Driver = &ics710Drivers[card];

    //convert raw integer data (32-bit) into voltages (double, OUTA)
    unsigned range = (unsigned) pics710Driver->gainControl.input_voltage_range;
    int *prawData = (int *) precord->a;
    double *poffset = (double *) precord->b;
    double *pcoeff = (double *) precord->c;
    double *pvoltData = (double *) precord->vala;
    for (unsigned int i = 0; i < pics710Driver->nSamples; i++)
    {
        pvoltData[i] = (prawData[i] / (256 * 8388608.0)) * (10.0 / (range + 1));
        pvoltData[i] = pvoltData[i] * (*pcoeff) - (*poffset);
    }
    //reset NELM and NORD of OUTA waveform record; plink is OUTA now
    plink = &precord->outa;
    assert((plink != NULL) && (DB_LINK == plink->type));
    paddr = (DBADDR *) plink->value.pv_link.pvt;
    waveformRecord *pwf = (waveformRecord *) paddr->precord;
    pwf->nelm = pics710Driver->nSamples;
    pwf->nord = pics710Driver->nSamples;
    //printf("prawData[0]: %d, pvoltData[0]: %f, coeff: %f, off: %f",
    //prawData[0], pvoltData[0], *pcoeff, *poffset);
    double max;
    double min;
    double sum;
    double ave;
    double std;
    double maxROI;
    double minROI;
    double sumROI;
    double aveROI;
    double stdROI;
    unsigned int *startP = (unsigned int *) precord->d;
    unsigned int *endP = (unsigned int *) precord->e;
    //reset starP and endP longout records; plink is INPD/INPE now
    longoutRecord *plongout;
    if (*endP > pwf->nord)
    {
        *endP = pwf->nord;
        plink = &precord->inpe;
        assert((plink != NULL) && (DB_LINK == plink->type));
        paddr = (DBADDR *) plink->value.pv_link.pvt;
        plongout = (longoutRecord *) paddr->precord;
        plongout->val = *endP;
        //plongout->proc = 1;//can't cause the record to be processed
        errlogPrintf("endPoint should <=pwf->nord (%d), reset \n", pwf->nord);
    }
    if (*startP >= *endP)
    {
        *startP = 0;
        plink = &precord->inpd;
        assert((plink != NULL) && (DB_LINK == plink->type));
        paddr = (DBADDR *) plink->value.pv_link.pvt;
        plongout = (longoutRecord *) paddr->precord;
        plongout->val = *startP;
        errlogPrintf("startPoint should < endPoint, reset \n");
    }

    //calculate max, min, sum/integral, mean, std in the whole waveform
    processBasic(pvoltData, 0, pwf->nord, &max, &min, &sum, &ave, &std);
    //data analysis on part of the waveform (ROI, region of interest)
    processBasic(pvoltData, *startP, *endP, &maxROI, &minROI, &sumROI, &aveROI,
            &stdROI);

    // put the calculated results into ai records
    *(double *) precord->valb = ave;
    *(double *) precord->valc = max;
    *(double *) precord->vald = min;
    *(double *) precord->vale = sum;
    *(double *) precord->valf = std;
    *(double *) precord->valg = aveROI;
    *(double *) precord->valh = maxROI;
    *(double *) precord->vali = minROI;
    *(double *) precord->valj = sumROI;
    *(double *) precord->valk = stdROI;
    //ROI length: ms
    unsigned int ROISamples = *endP - *startP;
    *(double *) precord->vall = ROISamples * (1.0 / pics710Driver->sampleRate);

    return (0);
}

/*see ics710Channel.db for INP/OUT fields
 * process buffered data: 60 samples (1-minute data for 1Hz injection)
 * */
static long
processBuf(aSubRecord *precord)
{
    assert(precord != NULL);

    //static unsigned shot = 0;
    //static unsigned event = 0;
    unsigned j = 0;
    double sum = 0.0;
    double ave = 0.0;
    double std = 0.0;
    static double curTime = 0.0;
    static double preTime = 0.0;
    static int shotPerSecondCalc = 0;
    double qRate = 0.0;

    //get StdVROIOverINOS, this is not moving average
    unsigned int interstedShots = *(unsigned int *) precord->b;
    //printf("interstedShots: %d \n", interstedShots);
    //unsigned *pShots = (unsigned *) precord->e;
    double *pbufV = (double *) precord->valc;
    unsigned *pshot = (unsigned *) precord->f;
    if (*pshot >= interstedShots)
    {
        for (j = 0; j < interstedShots; j++)
        {
            //sum += ((double *) precord->valc)[j];
            sum += pbufV[j];
        }
        ave = sum / interstedShots;
        //printf("sum: %f, ave is %f \n", sum, ave);
        for (j = 0; j < interstedShots; j++)
        {
            std += (pbufV[j] - ave) * (pbufV[j] - ave);
        }
        std = sqrt(std / interstedShots);
        //printf("rmsNoise is %f \n", std);
        //OUTA, "${MON}StdVROIOverINOS-I PP"
        *(double *) precord->vala = std;
        *pshot = 0;
    }

    //((double *) precord->valc)[*pShots] = *(double *) precord->a;
    pbufV[*pshot] = *(double *) precord->a;
    (*pshot)++;
    //printf("shot is %d \n", *pshot);


    //charge rate: nC/s, no-moving sum and moving sum
    double noMovingSumQ = 0.0;
    unsigned int shotsPerSecond = *(unsigned int *) precord->d;
    //printf("shotsPerSecond: %d \n", shotsPerSecond);
    unsigned *pevent = (unsigned *) precord->g;
    double *pbufQ = (double *) precord->vald;
    pbufQ[*pevent] = *(double *) precord->c;
    (pevent)++;
    if (*pevent >= shotsPerSecond)
    {
        *pevent = 0;
        for (j = 0; j < shotsPerSecond; j++)
        {
            noMovingSumQ += pbufQ[j];
        }
        *(double *) precord->valb = noMovingSumQ;
        //printf("sumQ: %f \n", sumQ);
    }

    //charge rate: moving summing Q in one second
    //double *pSumTime = (double *) precord->g; // pSumTime seems global variable
    double *pCircularQ = (double *) precord->e;
    //int *pShotPerSecond = (int *) precord->vale;
    double *pQRate = (double *) precord->vale;
    for (j = (10 - (shotsPerSecond)); j < 10; j++)
    {
        //printf("j=%d, pCircularQ[j]: %f \n", j, pCircularQ[j]);
        qRate += pCircularQ[j];
    }
    *pQRate = qRate;
    qRate = 0.0;
    //printf("QRate: %f \n", *pQRate);
    return (0);
}

//see ics710Card.db for INP/OUT fields
static long
createTimeAxis(aSubRecord *precord)
{
    assert(precord != NULL);
    double *ptimeAxis = (double *) precord->vala;

    unsigned long nSample;
    double sampleLength;
    //input links: number of samples(data points), sample length (N ms)
    //sizeof(unsigned long) == sizeof(int)
    memcpy(&nSample, (int *) precord->a, precord->noa * sizeof(int));
    memcpy(&sampleLength, (double *) precord->b, precord->nob * sizeof(double));

    //using effective number of samples(NELM) in the waveform for data analysis
    struct link *plink = &precord->outa;
    assert(DB_LINK == plink->type);
    struct dbAddr *paddr = (DBADDR *) plink->value.pv_link.pvt;
    waveformRecord *pwf = (waveformRecord *) paddr->precord;
    pwf->nelm = nSample;
    pwf->nord = nSample;
    //printf("number of effective samples(samples/ch): %d \n", pwf->nelm);

    for (unsigned int i = 0; i < pwf->nelm; i++)
    {
        ptimeAxis[i] = i * (sampleLength / nSample);
    }

    //printf("put all output links values in %s \n", precord->name);
    return (0);
}

/* Register these symbols for use by IOC code: */epicsRegisterFunction(processWf)
;
epicsRegisterFunction(processBuf)
;
epicsRegisterFunction(createTimeAxis)
;

