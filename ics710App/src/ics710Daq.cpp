/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>/*for microsecond: gettimeofday()*/
#include <math.h>
#include <errno.h>

#include "ics710Daq.h"
#include "ics710Drv.h"

#include "epicsThread.h"
#include "epicsTime.h"

/*global variable: defined in ics710DrvInit.cpp*/
extern double timeAfterADCInt;
extern double triggerRate;
extern double timeAfterRead;
extern double dcOffset[MAX_CHANNEL];
extern double inputRange[MAX_CHANNEL];

extern "C"
{
  /*split the raw DMA buffer data into the data of individual channel: raw integer and double voltage*/
  static int fileUnpackedData710 (ics710Driver *pics710Driver)
  {
	  unsigned channel = 0;
	  unsigned nSamples = 0;
	  int rawData;
	  //double rawVolt;
	  ics710Debug("split the raw DMA buffer data into the data of individual channel \n");

	  /* convert the raw data(32-bit long) to voltage, then calibrate it (slope=0.98) */
	  for (channel = 0; channel < pics710Driver->totalChannel; channel++)
	  {
		  for (nSamples = 0; nSamples < pics710Driver->nSamples; nSamples++)
		  {
			  if (0 != (channel % 2))
			  {
				  /* 1<<23 = 8388608, volt-volt/50 (50Ohm?), ch1DcOffset=0.173 for 10V Input Range */
				  //rawVolt = ((int)pics710Driver->pAcqData[nSamples * pics710Driver->totalChannel + channel - 1] / (256* 8388608.0)) * (10.00 / (1+pics710Driver->gainControl.input_voltage_range));
				  rawData = (int)pics710Driver->pAcqData[nSamples*pics710Driver->totalChannel+channel-1];
				  pics710Driver->rawData[channel][nSamples] = rawData;
				  pics710Driver->chData[channel][nSamples] = (rawData/(256* 8388608.0)) * (inputRange[channel]/(1+pics710Driver->gainControl.input_voltage_range)) + dcOffset[channel];
				  //pics710Driver->chData[channel][nSamples] = (rawData/(256* 8388608.0)) * (10.00/(1+pics710Driver->gainControl.input_voltage_range)) + dcOffset[channel];
				  //pics710Driver->chData[channel][nSamples] = 0.98 * rawVolt - dcOffset[channel];
			  }
			  else
			  {
				  /* 1<<23 = 8388608, volt-volt/50 (50Ohm?), ch0DcOffset=0.0481 for 10V Input Range */
				  //rawVolt = ((int)pics710Driver->pAcqData[nSamples * pics710Driver->totalChannel + channel + 1] / (256* 8388608.0)) * (10.00 / (1+pics710Driver->gainControl.input_voltage_range));
				  rawData = (int)pics710Driver->pAcqData[nSamples*pics710Driver->totalChannel+channel+1];
				  pics710Driver->rawData[channel][nSamples] = rawData;
				  pics710Driver->chData[channel][nSamples] = (rawData/(256* 8388608.0)) * (inputRange[channel]/(1+pics710Driver->gainControl.input_voltage_range)) + dcOffset[channel];
				  //pics710Driver->chData[channel][nSamples] = (rawData/(256* 8388608.0)) * (10.00/(1+pics710Driver->gainControl.input_voltage_range)) + dcOffset[channel];
				  //pics710Driver->chData[channel][nSamples] = rawVolt - dcOffset[channel];
				  //pics710Driver->chData[channel][nSamples] = 0.98 * rawVolt - dcOffset[channel];
 /* third-order polyfit: still get '0.98'
  * pics710Driver->chData[channel][nSamples] = -1.4662e-05*(rawVolt*rawVolt*rawVolt)+1.2958e-04*(rawVolt*rawVolt)+9.8039e-01*rawVolt-4.7776e-02;
	p21 = -1.4662e-05   1.2958e-04   9.8039e-01  -4.7776e-02
*/
			  }//if (0 != (channel % 2))

		  }// for (nSamples = 0; nSamples < pics710Driver->nSamples; nSamples++)

	  }// for (channel = 0; channel < pics710Driver->totalChannel; channel++)

	  return 0;
  }

  static int filePackedData710 (ics710Driver *pics710Driver)
  {
	  return 0;
  }

  /* one thread for each card DAQ*/
  void ics710DaqThread(void *arg)
  {
	 ics710Driver *pics710Driver = static_cast<ics710Driver*>(arg);
	 int errorCode;
     int timeout = 10; /* seconds */
     //char buf[30];
     epicsTimeStamp now;
     double oldTimeAfterADCInt = 0.0;

    while (1)
    {
		//ics710Debug("enter ics710DaqThread. \n");
		printf("enter ics710DaqThread. \n");
    	epicsThreadSleep(1.00);
    	epicsEventWait(pics710Driver->runSemaphore); /*runSemaphore is setup in ics710DrvMbbo/Longout.cpp*/
        //do
    	while (pics710Driver->running)/*running is setup in ics710DrvMbbo.cpp*/
        {
 daqStart:
 /*
 	 	 	epicsTimeGetCurrent(&now);
 	 	    epicsTimeToStrftime(buf, 30, "%Y/%m/%d %H:%M:%S.%06f", &now);
 	 	    printf("a new daq started at: %s \n", buf);
*/
 	 	 	 /*must buffer reset for continuous/multiple acquisitions, otherwise only can get one-short acquisition*/
			if (ICS710_OK != (errorCode = ics710BufferReset (pics710Driver->hDevice)))
			{
				printf("can't reset buffer register, errorCode: %d \n", errorCode);
				goto daqStart;
			}

			if (ICS710_TRIG_INTERNAL == pics710Driver->control.trigger_select)
			{
				if (ICS710_OK != (errorCode = ics710Trigger(pics710Driver->hDevice)))
				{
					printf ("can't software trigger the board, errorCode: %d \n", errorCode);
					goto daqStart;
				}
			}
/*
 	 	 	epicsTimeGetCurrent(&now);
 	 	    epicsTimeToStrftime(buf, 30, "%Y/%m/%d %H:%M:%S.%06f", &now);
 	 	    printf("buffer reseted at: %s \n", buf);
*/
			/* Enable ADC interrupt and wait for acquisition to be complete*/
			if (ICS710_OK != (errorCode = ics710WaitADCInt(pics710Driver->hDevice, &timeout)))
			{
				printf ("wait ADC interrupt timeout(%d seconds), errorCode: %d \n", timeout, errorCode);
				pics710Driver->timeouts++;
				goto daqStart;
			}
/*
 	 	    do
 	 	    {
 	 	    	errorCode = ics710WaitADCInt(pics710Driver->hDevice, &timeout);
 	 	    } while (EAGAIN == errno);
 	 	    if (EBUSY == errno) epicsThreadSleep(0.2);
*/
 	 	 	epicsTimeGetCurrent(&now);
 	 	 	timeAfterADCInt = now.secPastEpoch + now.nsec/1000000000.0;
 	 	 	triggerRate = 1.0/(timeAfterADCInt - oldTimeAfterADCInt);
            oldTimeAfterADCInt = timeAfterADCInt;
/*
 	 	 	epicsTimeGetCurrent(&now);
 	 	    epicsTimeToStrftime(buf, 30, "%Y/%m/%d %H:%M:%S.%06f", &now);
 	 	    printf("ADC interrupted and acquisition completed at: %s \n", buf);
*/
            /*Read out data via DMA*/
			epicsMutexLock(pics710Driver->daqMutex);
			if (0 > (errorCode = read(pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead)))
			{
				printf ("can't read the data, errorCode: %d \n", errorCode);
				epicsMutexUnlock(pics710Driver->daqMutex);
				goto daqStart;
			}
            epicsMutexUnlock(pics710Driver->daqMutex);
/*
 	 	    do
 	 	    {
 	 	    	errorCode = read(pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead);
 	 	    } while (EAGAIN == errno);
 	 	    if (EBUSY == errno) epicsThreadSleep(0.2);
 */
 	 	 	epicsTimeGetCurrent(&now);
 	 	 	timeAfterRead = now.secPastEpoch + now.nsec/1000000000.0;
/*
 	 	    epicsTimeToStrftime(buf, 30, "%Y/%m/%d %H:%M:%S.%06f", &now);
 	 	    printf("data readout from the board Via DMA at: %s \n", buf);
*/
 	 	  /*split the raw DMA buffer data into the data of individual channel*/
			if (pics710Driver->control.packed_data == 0)
				fileUnpackedData710(pics710Driver);
			else
				filePackedData710(pics710Driver);

            scanIoRequest(pics710Driver->ioscanpvt);
/*
            epicsTimeGetCurrent(&now);
            epicsTimeToStrftime(buf, 30, "%Y/%m/%d %H:%M:%S.%06f", &now);
            printf("ioscanpvt sent out at: %s \n", buf);
*/
            ics710Debug("scanIoRequest: send interrupt to waveform records (I/O Intr) \n");
            pics710Driver->count++;
            epicsEventSignal(pics710Driver->runSemaphore); /*runSemaphore is setup in ics710DrvMbbo(Longout).cpp*/

        } //while (pics710Driver->running);/*running is setup in ics710DrvMbbo.cpp*/

    }//while(1)

  }//void ics710DaqThread(void *arg)

}//extern "C"
