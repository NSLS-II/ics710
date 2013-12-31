/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */
/*ics710DrvInit.cpp: initialize the board: configure, create DAQ thread*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <fcntl.h> //open(), fcntl()
#include <unistd.h>

#include "epicsExport.h"
#include "iocsh.h"
#include "epicsThread.h"
#include "epicsPrint.h"
#include "epicsTime.h"
#include "drvSup.h"

#include "ics710Drv.h"

namespace
{
    const char rcs_id[] = "$Id$";
}

//global variable
ics710Driver ics710Drivers[MAX_DEV];

static unsigned int totalModule = 0;
//initial values of configurable parameters: can be overwritten by autosave
const int N_SAMPLES = 2000;
const int GAIN = 0;
const int FILTER = 1;
const double SAMPLING_RATE = 20.0;
//0:ICS710_SAMP_NORMAL(best SNR),1:*_DOUBLE,2:ICS710_SAMP_QUAD(fastest)
const int OSR = 2;
const int TRIG_SEL = 1;//ICS710_TRIG_INTERNAL or ICS710_TRIG_EXTERNAL
/*May-13-2011: it's better not to use Continuous acquisition mode
 * which sometimes gives glitch data;
 July-12-2011: verified on new boards, Continuous Mode doesn't work well
 **/
const int ACQ_MOD = 1;//ICS710_CAPTURE_NOPRETRG

//function forward declaration
static int
ics710Config(ics710Driver *pics710Driver);
static void
releaseBoard(ics710Driver *pics710Driver);
static void
ics710DaqThread(void *arg);
static int
splitUnpackedData710(ics710Driver *pics710Driver);

/*initialize individual board:
 * the first card is '0', totalChannel must be an even number;
 * 7 configurable parameters:
 * longout record for samples/ch,
 * ao for samplingRate;
 * mbbo for others: gain, filter, osr, triggerSel, acqMode.
 * reconfiguration of totalChannel is a expensive operation
 * including ADC recalibration, etc. just fix totalChannel = 8
 * */
static int
ics710Init(int card, int totalChannel)
{
    //assert(argv == 2);
    if ((card < 0) || (card > 7))
    {
        errlogPrintf("Error: card #%d is out of range [0, 7] \n", card);
        errlogPrintf("\t '0' is the first card, max. number of card is 8 \n");
        exit(1);
    }

    if ((totalChannel < 2) || (totalChannel > 32))
    {
        errlogPrintf("Error: totalChannel (%d) is out of range [2, 32] \n",
                totalChannel);
        exit(1);
    }
    if ((totalChannel % 2) != 0)
    {
        totalChannel = totalChannel + 1;
        errlogPrintf("Warning: totalChannel is reset to an even number %d\n",
                totalChannel);
    }

    //all data associated with individual card are in ics710Drivers[card]
    ics710Driver *pics710Driver = &ics710Drivers[card];
    pics710Driver->hDevice = -1;

    //get the handler for the card
    char devName[30];
    snprintf(devName, sizeof(devName), "/dev/ics710-%d", card + 1);
    pics710Driver->hDevice = open(devName, O_RDWR);

    if (0 > pics710Driver->hDevice)
    {
        errlogPrintf("Error: cann't open %s \n", devName);
        exit(1);
    }
    else
    {
        //blocking I/O: easy implementation, seems efficient for 10Hz IOC update
        fcntl(pics710Driver->hDevice, F_SETFL,
                fcntl(pics710Driver->hDevice, F_GETFL) & ~O_NONBLOCK);

        //default parameters: currently fixed in ics710 IOC
        pics710Driver->module = card;
        pics710Driver->totalChannel = totalChannel;
        pics710Driver->control.adc_clock_select = ICS710_CLOCK_INTERNAL;
        pics710Driver->control.diag_mode_enable = ICS710_DISABLE;
        pics710Driver->control.fpdp_enable = ICS710_DISABLE;
        pics710Driver->control.enable = ICS710_DISABLE;
        pics710Driver->control.int_trigger = ICS710_INACTIVE;
        pics710Driver->control.adc_hpfilter_enable = ICS710_DISABLE;
        pics710Driver->control.zero_cal = ICS710_ZCAL_INTERNAL;
        pics710Driver->control.packed_data = ICS710_UNPACKED_DATA;
        pics710Driver->control.system_master = ICS710_ENABLE;
        pics710Driver->control.adc_master = ICS710_ENABLE;
        pics710Driver->control.fpdp_master = ICS710_ENABLE;
        pics710Driver->control.adc_termin = ICS710_ENABLE;
        pics710Driver->control.fpdp_termin = ICS710_ENABLE;
        pics710Driver->control.extclk_term = ICS710_ENABLE;
        pics710Driver->control.extrig_term = ICS710_ENABLE;
        pics710Driver->control.extrig_mode = ICS710_EXTRIG_RISING;
        pics710Driver->masterControl.board_address = 0;

        //initial values of configurable parameters: can be overwritten by autosave
        pics710Driver->nSamples = N_SAMPLES;
        pics710Driver->gainControl.input_voltage_range = GAIN;
        pics710Driver->filterControl.cutoff_freq_range = FILTER;
        pics710Driver->control.oversamp_ratio = OSR;
        pics710Driver->ics710AdcClockRate = SAMPLING_RATE * (256 / (1 << OSR))
                / 1000;
        pics710Driver->control.trigger_select = TRIG_SEL;
        pics710Driver->control.acq_mode = ACQ_MOD;
        /*To synchronize external event, use external trigger(TRIG_SEL == 1)
         *and captureWithoutPreTrigger(ACQ_MOD == 1);
         *if having to use external trigger and Continuous Mode,
         * must set extrig_mode to level(high) control;
         **/
        if ((ICS710_TRIG_EXTERNAL == pics710Driver->control.trigger_select)
                && (ICS710_CONTINUOUS == pics710Driver->control.acq_mode))
            pics710Driver->control.extrig_mode = ICS710_EXTRIG_HIGH;

        //Calculated parameters: acquisition length, channel count, buffer length
        pics710Driver->acqLength = pics710Driver->nSamples - 1;
        //Channel Count Register, for un-packed data (32-bit)
        pics710Driver->channelCount = pics710Driver->totalChannel - 1;
        pics710Driver->masterControl.numbers_of_channels
                = pics710Driver->channelCount;
        //buffer length Register, for un-packed data (32-bit)
        pics710Driver->bufLength = (pics710Driver->totalChannel
                * pics710Driver->nSamples / 2) - 1;
        if (pics710Driver->bufLength > 262143)
        {
            errlogPrintf("Error: buf_len > 256K(4 MBytes memory board) \n");
            errlogPrintf("\t must reduce totalChannel or N_SAMPLES \n");
            exit(1);
        }

        //allocate DMA memory and clear it
        pics710Driver->bytesToRead = (pics710Driver->bufLength + 1) * 8;
        pics710Driver->pAcqData = (long *) ics710AllocateDmaBuffer(
                pics710Driver->hDevice, pics710Driver->bytesToRead);

        if (NULL == pics710Driver->pAcqData)
        {
            errlogPrintf("Error: can't allocate DMA memory to %u bytes \n",
                    pics710Driver->bytesToRead);
            exit(1);
        }
        memset(pics710Driver->pAcqData, 0, pics710Driver->bytesToRead);

        //configure the board
        if (0 != ics710Config(pics710Driver))
        {
            errlogPrintf("Error: can't configure board#%d \n", card);
            exit(1);
        }
        printf("ics710 card #%d is configured successfully\n ", card);

        pics710Driver->runSemaphore = epicsEventMustCreate(epicsEventEmpty);
        pics710Driver->daqMutex = epicsMutexMustCreate();
        pics710Driver->count = 0;
        scanIoInit(&pics710Driver->ioscanpvt); /*I/O Intr initialization*/

        //create a thread (ics710DaqThread) for individual card
        char name[30];
        snprintf(name, sizeof(name), "tics710DAQ%u", card);
        epicsThreadMustCreate(name, epicsThreadPriorityMin, 5000000,
                ics710DaqThread, pics710Driver);
        printf("DAQ thread (%s) is created for card #%d:\n ", name, card);
    }// if (0 > pics710Driver->hDevice) else

    totalModule++;
    return 0;
}//static int ics710Init(unsigned int card, unsigned int totalChannel)

/*register IOC shell*/
static const iocshArg ics710InitArg0 =
{ "card", iocshArgInt };
static const iocshArg ics710InitArg1 =
{ "totalChannel", iocshArgInt };
static const iocshArg * const ics710InitArgs[2] =
{ &ics710InitArg0, &ics710InitArg1 };
static const iocshFuncDef ics710InitFuncDef =
{ "ics710Init", 2, ics710InitArgs };

static void
ics710InitCallFunc(const iocshArgBuf *args)
{
    ics710Init(args[0].ival, args[1].ival);
}

void
ics710Registrar()
{
    iocshRegister(&ics710InitFuncDef, ics710InitCallFunc);
}
epicsExportRegistrar(ics710Registrar);

/* called by ics710DaqThread();
 * split the raw DMA buffer data into the data of individual channel:
 * only raw integer data (32-bit); the conversion to voltages is done by
 * aSub record in ics710ProcessWfAsub.cpp
 * */
static int
splitUnpackedData710(ics710Driver *pics710Driver)
{
    assert(pics710Driver != NULL);

    unsigned int channel = 0;
    unsigned int nSamples = 0;
    unsigned int samplePoint = 0;
    unsigned int totalChannel = pics710Driver->totalChannel;
    ics710Debug("split the raw DMA buffer data into individual channel \n");

    for (channel = 0; channel < pics710Driver->totalChannel; channel++)
    {
        for (nSamples = 0; nSamples < pics710Driver->nSamples; nSamples++)
        {
            if (0 != (channel % 2))
            {
                samplePoint = nSamples * totalChannel + channel - 1;
                pics710Driver->rawData[channel][nSamples]
                        = (int) pics710Driver->pAcqData[samplePoint];
            }
            else
            {
                samplePoint = nSamples * totalChannel + channel + 1;
                pics710Driver->rawData[channel][nSamples]
                        = (int) pics710Driver->pAcqData[samplePoint];
            }//if (0 != (channel % 2))
        }// for (nSamples = 0; nSamples < pics710Driver->nSamples; nSamples++)
    }// for (channel = 0; channel < pics710Driver->totalChannel; channel++)

    return 0;
}//static int splitUnpackedData710(ics710Driver *pics710Driver)

//called by ics710Init(): one thread for each card DAQ
static void
ics710DaqThread(void *arg)
{
    assert(arg != NULL);
    ics710Driver *pics710Driver = static_cast<ics710Driver*> (arg);

    int errorCode = -1;
    int timeout = 10; /* seconds */
    //char timestampText[30];
    epicsTimeStamp now;
    double curTimeAfterADCInt = 0.0;
    double preTimeAfterADCInt = 0.0;
    pics710Driver->readErrors = 0;
    pics710Driver->timeouts = 0;
    pics710Driver->count = 0;

    while (1)
    {
        //printf("enter ics710DaqThread. \n");
        epicsThreadSleep(1.00);
        //runSemaphore is setup in ics710DrvMbbo/Longout.cpp
        epicsEventWait(pics710Driver->runSemaphore);

        while (pics710Driver->running)
        {
            //must buffer reset for continuous/multiple acquisitions
            errorCode = ics710BufferReset(pics710Driver->hDevice);
            if (ICS710_OK != errorCode)
            {
                errlogPrintf("can't reset buffer of card#%d, errorCode: %d\n",
                        pics710Driver->module, errorCode);
            }

            if (ICS710_TRIG_INTERNAL == pics710Driver->control.trigger_select)
            {
                errorCode = ics710Trigger(pics710Driver->hDevice);
                if (ICS710_OK != errorCode)
                {
                    errlogPrintf("can't soft trigger card#%d, errorCode:%d\n",
                            pics710Driver->module, errorCode);
                }
            }

            //Enable ADC interrupt and wait for acquisition to be complete
            errorCode = ics710WaitADCInt(pics710Driver->hDevice, &timeout);
            if (ICS710_OK != errorCode)
            {
                errlogPrintf(
                        "card#%d wait ADC interrupt timeout(%d seconds), \
					errorCode: %d\n",
                        pics710Driver->module, timeout, errorCode);
                pics710Driver->timeouts++;
                pics710Driver->trigRate = 0.0;
            }
            else
            {
                //trigger rate / IOC update rate
                epicsTimeGetCurrent(&now);
                curTimeAfterADCInt = now.secPastEpoch + now.nsec / 1000000000.0;
                pics710Driver->trigRate = 1.0 / (curTimeAfterADCInt
                        - preTimeAfterADCInt);
                preTimeAfterADCInt = curTimeAfterADCInt;

                //Read out data via DMA
                epicsMutexLock(pics710Driver->daqMutex);
                errorCode = read(pics710Driver->hDevice,
                        pics710Driver->pAcqData, pics710Driver->bytesToRead);
                if (0 > errorCode)
                {
                    errlogPrintf(
                            "can't read data from card#%d, errorCode: %d\n",
                            pics710Driver->module, errorCode);
                    pics710Driver->readErrors++;
                }
                epicsMutexUnlock(pics710Driver->daqMutex);

                //split the raw DMA buffer data into the data of individual channel
                if (splitUnpackedData710(pics710Driver) != 0)
                {
                    errlogPrintf("can't split DMA buffer data of card#%d \n",
                            pics710Driver->module);
                }

                pics710Driver->count++;
            }//if (ICS710_OK != errorCode)
            scanIoRequest(pics710Driver->ioscanpvt);
            ics710Debug("send interrupt to waveform records of card#%d \n",
                    pics710Driver->module);

            epicsEventSignal(pics710Driver->runSemaphore);
        } //while (pics710Driver->running);
    }//while(1)
}//void ics710DaqThread(void *arg)

/*called by ics710Init(): configure the board,
 *17 Linux Device Driver API functions are used
 **/
static int
ics710Config(ics710Driver *pics710Driver)
{
    assert(pics710Driver != NULL);

    int errorCode = 0;
    double defaultADCFreq;
    double defaultFPDPFreq;
    double ics710FpdpRate = 30;
    double actualFPDPRate;
    unsigned long long fifoFrames = 1;
    unsigned long long dmaLocalSpace = 0; /*must be 0 for DMA*/
    time_t strobe;
    //2011-04-29: the lower ADC clock, the longer time for calibration;
    int timeout = 20;//20 seconds work for 1KS/s

    //must reset the board after power-up prior to any further operations
    if (ICS710_OK != (errorCode = ics710BoardReset(pics710Driver->hDevice)))
    {
        errlogPrintf("can't reset board, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    errorCode = ics710ADCFPDPDefaultClockSet(pics710Driver->hDevice,
            &defaultADCFreq, &defaultFPDPFreq);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't set ADC&FPDP clocks, errorCode: %d\n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    errorCode = ics710FPDPClockSet(pics710Driver->hDevice, &ics710FpdpRate,
            &actualFPDPRate);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't set FPDP clock, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    errorCode = ics710ADCClockSet(pics710Driver->hDevice,
            &pics710Driver->ics710AdcClockRate, &pics710Driver->actualADCRate);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't set ADC clock, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    errorCode = ics710ControlSet(pics710Driver->hDevice,
            &pics710Driver->control);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't set control register, errorCode: %d\n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    errorCode = ics710MasterControlSet(pics710Driver->hDevice,
            &(pics710Driver->masterControl));
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't set master control, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    errorCode = ics710GainSet(pics710Driver->hDevice,
            &pics710Driver->gainControl);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't set gain register, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    errorCode = ics710FilterSet(pics710Driver->hDevice,
            &(pics710Driver->filterControl));
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't set filter register, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    errorCode = ics710ChannelCountSet(pics710Driver->hDevice,
            &pics710Driver->channelCount);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't set channel count, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    errorCode = ics710BufferLengthSet(pics710Driver->hDevice,
            &(pics710Driver->bufLength));
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't set buffer length, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    errorCode = ics710AcquireCountSet(pics710Driver->hDevice,
            &(pics710Driver->acqLength));
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't set acquisition length, errorCode:%d\n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    errorCode = ics710FPDPFramesSet(pics710Driver->hDevice, &fifoFrames);
    if (ICS710_OK != errorCode)
    {
        printf("can't set FPDP frame register, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    errorCode = ics710DmaLocalSpaceSet(pics710Driver->hDevice, &dmaLocalSpace);
    if (ICS710_OK != errorCode)
    {
        errlogPrintf("can't set DMA local space, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    //Buffer Reset to load the new register values
    if (ICS710_OK != (errorCode = ics710BufferReset(pics710Driver->hDevice)))
    {
        printf("can't reset buffer register, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    //ADC Reset and begin Zero Calibration: must do ADC reset after board reset
    if (ICS710_OK != (errorCode = ics710ADCReset(pics710Driver->hDevice)))
    {
        errlogPrintf("can't reset ADC, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }
    strobe = time(NULL);
    do
    {
        usleep(10000);//delay 10ms
        errorCode = ics710StatusGet(pics710Driver->hDevice,
                &pics710Driver->stat);
        if (ICS710_OK != errorCode)
        {
            errlogPrintf("can't read status Reg, errorCode: %d \n", errorCode);
            releaseBoard(pics710Driver);
            return errorCode;
        }
        if ((time(NULL) - strobe) > timeout)
        {
            errlogPrintf("ADC calibration timeout: %d seconds \n", timeout);
            releaseBoard(pics710Driver);
            return errorCode;
        }

    }
    while (0 != pics710Driver->stat.cal);

    //start to perform acquisitions: ADCs continuously run, never stop
    if (ICS710_OK != (errorCode = ics710Enable(pics710Driver->hDevice)))
    {
        errlogPrintf("can't enable the board, errorCode: %d \n", errorCode);
        releaseBoard(pics710Driver);
        return errorCode;
    }

    //must issue a write to Arm Register if ACQ_MOD is 'capture with pre-trigger'
    if (ICS710_CAPTURE_WITHPRETRG == pics710Driver->control.acq_mode)
    {
        if (ICS710_OK != (errorCode = (ics710Arm(pics710Driver->hDevice))))
        {
            errlogPrintf("can't arm the board, errorCode: %d \n", errorCode);
            releaseBoard(pics710Driver);
            return errorCode;
        }
        epicsThreadSleep(2.00);
    }

    return errorCode;
}// int ics710Config(ics710Driver *pics710Driver)

//called by ics710Config(): free card resource
static void
releaseBoard(ics710Driver *pics710Driver)
{
    assert(pics710Driver != NULL);

    if ((0 < pics710Driver->hDevice) && (NULL != pics710Driver->pAcqData))
    {
        errlogPrintf("free card#%d resource \n", pics710Driver->module);
        ics710FreeDmaBuffer(pics710Driver->hDevice, pics710Driver->pAcqData,
                pics710Driver->bytesToRead);
        pics710Driver->pAcqData = NULL;
        close(pics710Driver->hDevice);
        pics710Driver->hDevice = -1;
    }
}

static long
report(int level)
{
    if (level >= 1)
    {
        printf("%d ics710 digitizers found! \n", totalModule);
    }

    if (level >= 2)
    {
        ics710Driver *pics710Driver;
        printf("********************************************************\n");
        for (unsigned int module = 0; module < totalModule; module++)
        {
            pics710Driver = &ics710Drivers[module];
            printf("digitizer #%d:\n", module);
            printf("\tsampling rate: %fHz, samples/ch: %d\n",
                    pics710Driver->sampleRate, pics710Driver->nSamples);
            printf("********************************************************\n");
        }//for (int module=0; module<nbr_acqiris_drivers; module++)    }
    }

    return 0;
}

static struct
{
    long number;
    DRVSUPFUN report;
    DRVSUPFUN init;
} drvIcs710 =
{ 2, (DRVSUPFUN) report, NULL };
epicsExportAddress(drvet,drvIcs710)
;

