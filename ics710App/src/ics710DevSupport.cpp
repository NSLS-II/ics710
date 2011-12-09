/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */
/* device support for longout, longin, mbbo, mbbi, ao, ai, waveform.
 * waveform: read raw integer data (32-bit) of individual channel;
 * longout: setSamples;
 * longin: readback samples/channel, read errors, timeout;
 * mbbo:    gain, cut-off frequency, osr, trigger mode,
 *          acquisition mode, start/stop;
 * mbbi:    readback: gain, cut-off frequency, osr, trigger mode,
 *          acquisition mode;
 * ao:       setSamplingRate;
 * ai:       getSamplingRate,  getTrigRate;
 * See xxxRecord.c in EPICS base for dset associated with return value
 * of init_record, read_xxx, write_xxx
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dbAccess.h"
#include "recGbl.h"
#include "devSup.h"
#include "epicsExport.h"
#include "alarm.h"
#include "link.h"
#include "epicsPrint.h"
#include "dbCommon.h"
#include "waveformRecord.h"
#include "longoutRecord.h"
#include "longinRecord.h"
#include "mbboRecord.h"
#include "mbbiRecord.h"
#include "aoRecord.h"
#include "aiRecord.h"

#include "ics710Drv.h"
#include "ics710Dev.h"

/* initialize record: check link type and link string, save CARD & CHANNEL
 * info in precord->dpvt
 * */
static long
ics710InitRecord(struct dbCommon *precord, DBLINK link)
{
    assert(precord != NULL);

    if (link.type != INST_IO)
    {
        recGblRecordError(S_db_badField, precord, "must be INST_IO link type");
        return S_db_badField;
    }

    struct instio* pinstio = &link.value.instio;
    if (!pinstio->string)
    {
        recGblRecordError(S_db_badField, precord, "link must not be empty");
        return S_db_badField;
    }

    ics710RecPrivate *pics710RecPrivate = new ics710RecPrivate;
    if (NULL == pics710RecPrivate)
    {
        errlogPrintf("Error: out of memory, can't init record \n");
        return -1;
    }

    //parse the link string: must have 'card'
    const char* sinp = pinstio->string;
    int status = sscanf(sinp, "C%u S%u", &pics710RecPrivate->card,
            &pics710RecPrivate->channel);
    if (status != 2)
    {
        status = sscanf(sinp, "C%u", &pics710RecPrivate->card);
        if (status != 1)
        {
            delete pics710RecPrivate;
            recGblRecordError(S_db_badField, precord, "cannot parse INP field");
            return S_db_badField;
        }
    }
    //save the parsed results: #card(0), #channel(0)
    precord->dpvt = pics710RecPrivate;

    return 0;
}

//I/O Intr: returns: (-1,0)=>(failure,success)
static long
ics710GetIointInfo(int cmd, dbCommon *precord, IOSCANPVT *ppvt)
{
    assert(precord != NULL);
    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    *ppvt = pics710Driver->ioscanpvt;
    return 0;
}

/*device support for waveform: get the raw integer data (32-bit);
 * converting raw data to calibrated voltages is done by aSub,
 * see processWf() in ics710ProcessWfAsub.cpp.
 * */

//returns: (-1,0)=>(failure,success)
static long
init_waveform(waveformRecord *precord)
{
    assert(precord != NULL);
    return (ics710InitRecord((struct dbCommon *) precord, precord->inp));
}

//returns: (-1,0)=>(failure,success)
static long
wfGetIointInfo(int cmd, waveformRecord *precord, IOSCANPVT *ppvt)
{
    assert(precord != NULL);
    /*
     ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
     int module = pics710RecPrivate->card;
     //global variable:ics710Drivers[]
     ics710Driver *pics710Driver = &ics710Drivers[module];

     *ppvt = pics710Driver->ioscanpvt;
     return 0;
     */
    return (ics710GetIointInfo(cmd, (dbCommon *) precord, ppvt));
}

//returns: (-1,0)=>(failure,success)
static long
readRawData(waveformRecord *precord)
{
    assert(precord != NULL);
    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    int channel = pics710RecPrivate->channel;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    unsigned int nSamples = pics710Driver->nSamples;
    memcpy(precord->bptr, &pics710Driver->rawData[channel][0], sizeof(int)
            * nSamples);

    precord->nelm = nSamples;
    precord->nord = nSamples;
    return 0;
}

dsetCommon devWfReadRawData =
{ 5, NULL, NULL, (DEVSUPFUN) init_waveform, (DEVSUPFUN) wfGetIointInfo,
        (DEVSUPFUN) readRawData };
epicsExportAddress(dset, devWfReadRawData)
;

/*device support for ao: configure sampling rate
 * */

//returns: (0,2)=>(success,success no convert)
static long
init_ao(aoRecord *precord)
{
    assert(precord != NULL);
    int status = ics710InitRecord((struct dbCommon *) precord, precord->out);
    if (status == 0)
    {
        status = 2;
    }
    return (status);
}

///(0)=>(success)
static long
setSampleRate(aoRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    double val = precord->val;
    if (abs((1000 * pics710Driver->ics710AdcClockRate / (256.0 / (1
            << pics710Driver->control.oversamp_ratio)) - val)) < 1.00)
        return 0;

    pics710Driver->ics710AdcClockRate = val * (256.0 / (1
            << pics710Driver->control.oversamp_ratio)) / 1000.0;
    if (pics710Driver->ics710AdcClockRate < 0.256)
    {
        pics710Driver->ics710AdcClockRate = 0.256;
        val = pics710Driver->ics710AdcClockRate / ((256.0 / (1
                << pics710Driver->control.oversamp_ratio)) / 1000.0);
        errlogPrintf("sampling rate is too low, reset to %f KHz \n", val);
    }
    if (pics710Driver->ics710AdcClockRate > 13.824)
    {
        pics710Driver->ics710AdcClockRate = 13.824;
        val = pics710Driver->ics710AdcClockRate / ((256.0 / (1
                << pics710Driver->control.oversamp_ratio)) / 1000.0);
        errlogPrintf("sampling rate is too high, reset to %f KHz \n", val);
    }

    if (ICS710_OK != (errorCode = ics710ADCClockSet(pics710Driver->hDevice,
            &pics710Driver->ics710AdcClockRate, &pics710Driver->actualADCRate)))
    {
        errlogPrintf("can't reconfigure ADC clock to %fKHz, error: %d \n", val,
                errorCode);
        return errorCode;
    }

    pics710Driver->sampleRate = val;
    precord->val = val;
    printf("reconfigure data output rate by setSamplingRate: %f KHz \n", val);

    return 0;
}

dsetCommon devAoSetSampleRate =
{ 6, NULL, NULL, (DEVSUPFUN) init_ao, NULL, (DEVSUPFUN) setSampleRate, NULL };
epicsExportAddress(dset, devAoSetSampleRate)
;

/*device support for ai: read back sampling rate, trigger / IOC update rate
 * */

//returns: (-1,0)=>(failure,success)
static long
init_ai(aiRecord *precord)
{
    assert(precord != NULL);
    return (ics710InitRecord((struct dbCommon *) precord, precord->inp));
    /*
     int status = ics710InitRecord((struct dbCommon *) precord, precord->inp);
     if (status == 0)
     {
     status = 2;
     }
     return (status);
     */
}

//(0,2)=> success and convert,don't convert)
//if convert then raw value stored in rval
static long
getSampleRate(aiRecord *precord)
{
    assert(precord != NULL);

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    //precord->val = pics710Driver->actualADCRate;
    precord->val = pics710Driver->actualADCRate / ((256.0 / (1
            << pics710Driver->control.oversamp_ratio)) / 1000.0);

    return (2);
}

dsetCommon devAiGetSampleRate =
{ 6, NULL, NULL, (DEVSUPFUN) init_ai, NULL, (DEVSUPFUN) getSampleRate, NULL };
epicsExportAddress(dset, devAiGetSampleRate)
;

//returns: (-1,0)=>(failure,success)
static long
aiGetIointInfo(int cmd, aiRecord *precord, IOSCANPVT *ppvt)
{
    assert(precord != NULL);
    return (ics710GetIointInfo(cmd, (dbCommon *) precord, ppvt));
}

//(0,2)=> success and convert,don't convert)
//if convert then raw value stored in rval
static long
getTrigRate(aiRecord *precord)
{
    assert(precord != NULL);

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    precord->val = pics710Driver->trigRate;
    //return 0;
    return 2;
}

dsetCommon devAiGetTrigRate =
{ 6, NULL, NULL, (DEVSUPFUN) init_ai, (DEVSUPFUN) aiGetIointInfo,
        (DEVSUPFUN) getTrigRate, NULL };
epicsExportAddress(dset, devAiGetTrigRate)
;

/*device support for mbbo: configure gain, cut-off frequency, osr,
 * triggerSel, acquisition mode, start/stop
 * */

//returns: (0,2)=>(success,success no convert)
static long
init_mbbo(mbboRecord *precord)
{
    assert(precord != NULL);
    int status = ics710InitRecord((struct dbCommon *) precord, precord->out);
    if (status == 0)
    {
        status = 2;
    }
    return (status);
}

//returns: (0,2)=>(success,success no convert)
static long
setGain(mbboRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    unsigned long long val = (unsigned long long) precord->val;
    if (val == pics710Driver->gainControl.input_voltage_range)
        return 0;

    pics710Driver->gainControl.input_voltage_range = val;
    if (ICS710_OK != (errorCode = ics710GainSet(pics710Driver->hDevice,
            &(pics710Driver->gainControl))))
    {
        errlogPrintf("can't reset gain, errorCode: %d \n", errorCode);
        return errorCode;
    }

    printf("reconfigure gain/input voltage range to: %.3fV \n", 10.00 / (1
            + pics710Driver->gainControl.input_voltage_range));
    return 2;
}

dsetCommon devMbboSetGain =
{ 5, NULL, NULL, (DEVSUPFUN) init_mbbo, NULL, (DEVSUPFUN) setGain };
epicsExportAddress(dset, devMbboSetGain)
;

//returns: (0,2)=>(success,success no convert)
static long
setFilter(mbboRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    unsigned long long val = (unsigned long long) precord->val;
    if (pics710Driver->filterControl.cutoff_freq_range == val)
        return 0;

    pics710Driver->filterControl.cutoff_freq_range = val;
    if (ICS710_OK != (errorCode = ics710FilterSet(pics710Driver->hDevice,
            &(pics710Driver->filterControl))))
    {
        errlogPrintf("can't reset filter, errorCode: %d \n", errorCode);
        return errorCode;
    }

    printf("reconfigure filter/cut-off frequency to: %dKHz\n", 10
            * pics710Driver->filterControl.cutoff_freq_range);
    return 2;
}

dsetCommon devMbboSetFilter =
{ 5, NULL, NULL, (DEVSUPFUN) init_mbbo, NULL, (DEVSUPFUN) setFilter };
epicsExportAddress(dset, devMbboSetFilter)
;

//returns: (0,2)=>(success,success no convert)
static long
setOsr(mbboRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    unsigned long long val = (unsigned long long) precord->val;
    if (pics710Driver->control.oversamp_ratio == val)
        return 0;

    pics710Driver->control.oversamp_ratio = val;
    if (ICS710_OK != (errorCode = ics710ControlSet(pics710Driver->hDevice,
            &(pics710Driver->control))))
    {
        errlogPrintf("can't reset over-sampling ratio, error: %d\n", errorCode);
        return errorCode;
    }
    printf("reconfigure data output rate by setOsr to %d: \n", (int) val);

    //must re-enable board to start DAQ after reconfigure control register
    if (ICS710_OK != (errorCode = ics710Enable(pics710Driver->hDevice)))
    {
        errlogPrintf("setOsr: can't enable board, error: %d\n", errorCode);
        return errorCode;
    }

    return 2;
}

dsetCommon devMbboSetOsr =
{ 5, NULL, NULL, (DEVSUPFUN) init_mbbo, NULL, (DEVSUPFUN) setOsr };
epicsExportAddress(dset, devMbboSetOsr)
;

static long
setTrigMode(mbboRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    unsigned long long val = (unsigned long long) precord->val;
    if (pics710Driver->control.trigger_select == val)
        return 0;

    pics710Driver->control.trigger_select = val;
    if (ICS710_OK != (errorCode = ics710ControlSet(pics710Driver->hDevice,
            &(pics710Driver->control))))
    {
        errlogPrintf("can't reset trigMode, errorCode: %d \n", errorCode);
        return errorCode;
    }
    printf("reset trigger to: %s \n", (0 == val) ? "internal" : "external");

    //must re-enable board to start DAQ after reconfigure control register
    if (ICS710_OK != (errorCode = ics710Enable(pics710Driver->hDevice)))
    {
        errlogPrintf("setTrigger: can't enable board, errorCode: %d \n",
                errorCode);
        return errorCode;
    }

    return 2;
}

dsetCommon devMbboSetTrigMode =
{ 5, NULL, NULL, (DEVSUPFUN) init_mbbo, NULL, (DEVSUPFUN) setTrigMode };
epicsExportAddress(dset, devMbboSetTrigMode)
;

/* May-13-2011: it's better not to use Continuous acquisition mode
 * which sometimes gives glitch data;
 * July-12-2011:verified on new boards, Continuous Mode doesn't work well
 * */
//returns: (0,2)=>(success,success no convert)
static long
setAcqMode(mbboRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    unsigned long long val = (unsigned long long) precord->val;
    if (0 == val)
        val = 1;
    if (val == pics710Driver->control.acq_mode)
        return 0;

    pics710Driver->control.acq_mode = val;
    if (ICS710_OK != (errorCode = ics710ControlSet(pics710Driver->hDevice,
            &(pics710Driver->control))))
    {
        errlogPrintf("can't reset Acquisition Mode, error: %d \n", errorCode);
        return errorCode;
    }
    printf("reconfigure Acquisition Mode to: %s \n", (0
            == pics710Driver->control.acq_mode) ? "Continuous" : "Capture");

    //must re-enable board to start DAQ after reconfigure control register
    if (ICS710_OK != (errorCode = ics710Enable(pics710Driver->hDevice)))
    {
        errlogPrintf("setAcqMode: can't enable board, error: %d \n", errorCode);
        return errorCode;
    }

    return 2;
}

dsetCommon devMbboSetAcqMode =
{ 5, NULL, NULL, (DEVSUPFUN) init_mbbo, NULL, (DEVSUPFUN) setAcqMode };
epicsExportAddress(dset, devMbboSetAcqMode)
;

//returns: (0,2)=>(success,success no convert)
static long
setRunning(mbboRecord *precord)
{
    assert(precord != NULL);

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    unsigned int val = (unsigned int) precord->val;
    bool start_run = !pics710Driver->running && val;
    pics710Driver->running = val;
    if (start_run)
    {
        printf("start/restart DAQ \n");
        pics710Driver->count = 0;
        pics710Driver->timeouts = 0;
        pics710Driver->readErrors = 0;
        epicsEventSignal(pics710Driver->runSemaphore);
    }
    else
    {
        printf("DAQ is stopped\n");
    }

    return 2;
}

dsetCommon devMbboSetRunning =
{ 5, NULL, NULL, (DEVSUPFUN) init_mbbo, NULL, (DEVSUPFUN) setRunning };
epicsExportAddress(dset, devMbboSetRunning)
;

/*device support for mbbi: read back settings of gain, cut-off frequency, osr,
 * triggerSel, acquisition mode
 * */
//returns: (-1,0)=>(failure,success)
static long
init_mbbi(mbbiRecord *precord)
{
    assert(precord != NULL);
    return (ics710InitRecord((struct dbCommon *) precord, precord->inp));
    /*
     int status = ics710InitRecord((struct dbCommon *) precord, precord->inp);
     if (status == 0)
     {
     status = 2;
     }
     return (status);
     */
}

//(0,2)=>(success, success no convert)
static long
getGain(mbbiRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    ICS710_GAIN gainControl;
    errorCode = ics710GainGet(pics710Driver->hDevice, &gainControl);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't read gainControl, errorCode: %d \n", errorCode);
        return errorCode;
    }

    precord->val = (unsigned short) gainControl.input_voltage_range;
    //printf("getGain: %d \n", precord->val);
    return 2;
}

dsetCommon devMbbiGetGain =
{ 5, NULL, NULL, (DEVSUPFUN) init_mbbi, NULL, (DEVSUPFUN) getGain };
epicsExportAddress(dset, devMbbiGetGain)
;

//(0,2)=>(success, success no convert)
static long
getFilter(mbbiRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    ICS710_FILTER filterControl;
    ics710FilterGet(pics710Driver->hDevice, &filterControl);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't read filterControl, errorCode: %d \n", errorCode);
        return errorCode;
    }

    precord->val = (unsigned short) filterControl.cutoff_freq_range;
    return 2;
}

dsetCommon devMbbiGetFilter =
{ 5, NULL, NULL, (DEVSUPFUN) init_mbbi, NULL, (DEVSUPFUN) getFilter };
epicsExportAddress(dset, devMbbiGetFilter)
;

//(0,2)=>(success, success no convert)
static long
getOsr(mbbiRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    ICS710_CONTROL control;
    errorCode = ics710ControlGet(pics710Driver->hDevice, &control);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't read OSR, errorCode: %d\n", errorCode);
        return errorCode;
    }

    precord->val = (unsigned short) control.oversamp_ratio;
    return 2;
}

dsetCommon devMbbiGetOsr =
{ 5, NULL, NULL, (DEVSUPFUN) init_mbbi, NULL, (DEVSUPFUN) getOsr };
epicsExportAddress(dset, devMbbiGetOsr)
;

//(0,2)=>(success, success no convert)
static long
getTrigMode(mbbiRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    ICS710_CONTROL control;
    errorCode = ics710ControlGet(pics710Driver->hDevice, &control);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't read trigMode, errorCode: %d\n", errorCode);
        return errorCode;
    }

    precord->val = (unsigned short) control.trigger_select;
    return 2;
}

dsetCommon devMbbiGetTrigMode =
{ 5, NULL, NULL, (DEVSUPFUN) init_mbbi, NULL, (DEVSUPFUN) getTrigMode };
epicsExportAddress(dset, devMbbiGetTrigMode)
;

//(0,2)=>(success, success no convert)
static long
getAcqMode(mbbiRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    ICS710_CONTROL control;
    errorCode = ics710ControlGet(pics710Driver->hDevice, &control);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't read trigMode, errorCode: %d\n", errorCode);
        return errorCode;
    }

    precord->val = (unsigned short) control.acq_mode;
    //return 0;
    return 2;
}

dsetCommon devMbbiGetAcqMode =
{ 5, NULL, NULL, (DEVSUPFUN) init_mbbi, NULL, (DEVSUPFUN) getAcqMode };
epicsExportAddress(dset, devMbbiGetAcqMode)
;

/*device support for longout: configure number of samples;
 * */

//returns: (-1,0)=>(failure,success)
static long
init_longout(longoutRecord *precord)
{
    assert(precord != NULL);
    return (ics710InitRecord((struct dbCommon *) precord, precord->out));
}

//(-1,0)=>(failure,success)
static long
setSamples(longoutRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    unsigned int val = precord->val;
    if (val == pics710Driver->nSamples)
        return 0;

    //epicsEventWait(pics710Driver->runSemaphore);
    if (val < 1)
    {
        val = 1;
        errlogPrintf("nbrSamples should be > = 1: set Samples = 1 \n");
    }
    if (val > 262144)
    {
        val = 262144;
        errlogPrintf("nbrSamples should be < = 262144: set Samples = 262144 \n");
    }

    pics710Driver->bufLength = (val * pics710Driver->totalChannel / 2) - 1;
    if (pics710Driver->bufLength > 262143)
    {
        pics710Driver->bufLength = 262143;
        val = (262144 * 2) / pics710Driver->totalChannel;
        errlogPrintf("setSamples: buf_len > 256K, reset to %d\n", val);
    }

    pics710Driver->nSamples = val;
    pics710Driver->acqLength = pics710Driver->nSamples - 1;
    pics710Driver->bufLength = (val * pics710Driver->totalChannel / 2) - 1;

    //free the previous DMA memory and then re-allocate it
    if (ics710FreeDmaBuffer(pics710Driver->hDevice, pics710Driver->pAcqData,
            pics710Driver->bytesToRead))
    {
        errlogPrintf("setSamples: can't free DMA memory \n");
        //epicsMutexUnlock(pics710Driver->daqMutex);
        return -1;
    }
    pics710Driver->bytesToRead = (unsigned int) (pics710Driver->bufLength + 1)
            * 8;
    if (NULL == (pics710Driver->pAcqData = (long *) ics710AllocateDmaBuffer(
            pics710Driver->hDevice, pics710Driver->bytesToRead)))
    {
        errlogPrintf("setSamples: can't re-allocate DMA memory %u bytes\n",
                pics710Driver->bytesToRead);
        return -1;
    }
    memset(pics710Driver->pAcqData, 0, pics710Driver->bytesToRead);

    if (ICS710_OK != (errorCode = ics710BufferLengthSet(pics710Driver->hDevice,
            &(pics710Driver->bufLength))))
    {
        errlogPrintf("setSamples: can't reconfigure buffer length, error:%d\n",
                errorCode);
        return errorCode;
    }
    if (ICS710_OK != (errorCode = ics710AcquireCountSet(pics710Driver->hDevice,
            &(pics710Driver->acqLength))))
    {
        errlogPrintf("setSamples: can't reconfigure ACQ length, error: %d\n",
                errorCode);
        return errorCode;
    }
    if (ICS710_OK != (errorCode = ics710BufferReset(pics710Driver->hDevice)))
    {
        errlogPrintf("setSamples: can't reset buffer, error: %d\n", errorCode);
        return errorCode;
    }

    precord->val = val;
    printf("reconfigure nbrSamples to %d samples\n", val);
    //epicsEventSignal(pics710Driver->runSemaphore);

    return 0;
}

dsetCommon devLoSetSamples =
{ 5, NULL, NULL, (DEVSUPFUN) init_longout, NULL, (DEVSUPFUN) setSamples };
epicsExportAddress(dset, devLoSetSamples)
;

/*device support for longin: rea back number of samples;
 * */

//returns: (-1,0)=>(failure,success)
static long
init_longin(longinRecord *precord)
{
    assert(precord != NULL);
    return (ics710InitRecord((struct dbCommon *) precord, precord->inp));
}

//returns: (-1,0)=>(failure,success)
static long
liGetIointInfo(int cmd, waveformRecord *precord, IOSCANPVT *ppvt)
{
    assert(precord != NULL);
    return (ics710GetIointInfo(cmd, (dbCommon *) precord, ppvt));
}

//returns: (-1,0)=>(failure,success)
static long
getSamples(longinRecord *precord)
{
    assert(precord != NULL);
    int errorCode = 0;

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    unsigned long long acqLength;
    errorCode = ics710AcquireCountGet(pics710Driver->hDevice, &acqLength);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("getSamples: can't get ACQ length, error: %d\n", errorCode);

    }

    precord->val = (int) acqLength + 1;
    return 0;
}

dsetCommon devLiGetSamples =
{ 5, NULL, NULL, (DEVSUPFUN) init_longin, NULL, (DEVSUPFUN) getSamples };
epicsExportAddress(dset, devLiGetSamples)
;

//returns: (-1,0)=>(failure,success)
static long
getReadErrors(longinRecord *precord)
{
    assert(precord != NULL);

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    precord->val = (int) pics710Driver->readErrors;
    return 0;
}

dsetCommon devLiGetReadErrors =
{ 5, NULL, NULL, (DEVSUPFUN) init_longin, (DEVSUPFUN) liGetIointInfo,
        (DEVSUPFUN) getReadErrors };
epicsExportAddress(dset, devLiGetReadErrors)
;

//returns: (-1,0)=>(failure,success)
static long
getTimeouts(longinRecord *precord)
{
    assert(precord != NULL);

    ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *) precord->dpvt;
    int module = pics710RecPrivate->card;
    //global variable:ics710Drivers[]
    ics710Driver *pics710Driver = &ics710Drivers[module];

    precord->val = (int) pics710Driver->timeouts;
    return 0;
}

dsetCommon devLiGetTimeouts =
{ 5, NULL, NULL, (DEVSUPFUN) init_longin, (DEVSUPFUN) liGetIointInfo,
        (DEVSUPFUN) getTimeouts };
epicsExportAddress(dset, devLiGetTimeouts)
;
