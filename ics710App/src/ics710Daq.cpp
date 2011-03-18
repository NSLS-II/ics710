/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>//for microsecond: gettimeofday()
#include <math.h>

#include "ics710Daq.h"
#include "ics710Drv.h"

#include "ics710api.h"

#include "epicsThread.h"
#include "epicsTime.h"

/*global variable: defined in ics710DrvInit.cpp*/
//extern epicsTimeStamp startTime;
extern double timeAtLoopStart;
extern double timeAfterADCInt;
extern double triggerRate;
extern double timeAfterRead;

extern "C"
{
  //epicsTimeStamp startTime;
  //static int fileUnpackedData710 (pics710Driver->pAcqData, pics710Driver->totalChannel, i, pics710Driver->nSamples)
  static int fileUnpackedData710 (ics710Driver *pics710Driver)
  {
	  unsigned channel = 0;
	  unsigned nSamples = 0;
	  double rawVolt;

	  ics710Debug("split the unpacked data into different channel data \n");
	  for (channel = 0; channel < pics710Driver->totalChannel; channel++)
	  {
		  for (nSamples = 0; nSamples < pics710Driver->nSamples; nSamples++)
		  {
			  if (0 != (channel % 2))
			  {
				  //1<<23 = 8388608, volt-volt/50 (50Ohm?), ch1DcOffset=0.046 for 10V Input Range
				  rawVolt = ((int)pics710Driver->pAcqData[nSamples * pics710Driver->totalChannel + channel - 1] / (256* 8388608.0)) * (10.00 / (1+pics710Driver->gainControl.input_voltage_range));
				  pics710Driver->chData[channel][nSamples] = rawVolt;
				  //pics710Driver->chData[channel][nSamples] = rawVolt*(1-1/(50*(1+pics710Driver->gainControl.input_voltage_range))) - 0.046;
			  }
			  else
			  {
				  //1<<23 = 8388608, volt-volt/50 (50Ohm?), ch1DcOffset=0.0481
				  rawVolt = ((int)pics710Driver->pAcqData[nSamples * pics710Driver->totalChannel + channel + 1] / (256* 8388608.0)) * (10.00 / (1+pics710Driver->gainControl.input_voltage_range));
				  pics710Driver->chData[channel][nSamples] = rawVolt;
				  //pics710Driver->chData[channel][nSamples] = 0.98*rawVolt - 0.046;
			  }

			  //(pics710Driver->chData[channel])++;
		  }//		  for (nSamples = 0; nSamples < pics710Driver->nSamples; nSamples++)

	  }//	  for (channel = 0; channel < pics710Driver->totalChannel; channel++)

	  return 0;
  }

  static int filePackedData710 (ics710Driver *pics710Driver)
  {
	  return 0;
  }

  void ics710DaqThread(void *arg)
  {
	 ics710Driver *pics710Driver = static_cast<ics710Driver*>(arg);
	 int errorCode;
     int timeout = 5; /* seconds */
     char buf[30];
     epicsTimeStamp now;
     double oldTimeAfterADCInt;
     int i = 0;
     //pics710Driver->running = 1;

    while (1)
    {
		ics710Debug("enter ics710DaqThread. \n");
    	epicsThreadSleep(1.00);
    	epicsEventWait(pics710Driver->runSemaphore);
        do
        {
 daqStart:
 	 	 	 // strange: timeAtLoopStart > timeAfterADCInt?
 	 	 	epicsTimeGetCurrent(&now);
 	 	 	epicsTimeToStrftime(buf, 30, "%Y/%m/%d %H:%M:%S.%06f", &now);
 	 	 	//printf("DAQ start: %s \n", buf);
	 	 	timeAtLoopStart = now.secPastEpoch + now.nsec/1000000000.0;

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

			if (ICS710_OK != (errorCode = ics710WaitADCInt(pics710Driver->hDevice, &timeout)))
			{
				printf ("wait ADC interrupt timeout(%d seconds), errorCode: %d \n", timeout, errorCode);
				pics710Driver->timeouts++;
				goto daqStart;
			}
 	 	 	//epicsTimeGetCurrent(&startTime);
 	 	 	epicsTimeGetCurrent(&now);
 	 	 	epicsTimeToStrftime(buf, 30, "%Y/%m/%d %H:%M:%S.%06f", &now);
 	 	 	//printf("ADC Intrr: %s \n", buf);
 	 	 	timeAfterADCInt = now.secPastEpoch + now.nsec/1000000000.0;
 	 	 	//printf("total loop time: %f seconds \n", (timeAfterADCInt - oldTimeAfterADCInt));
 	 	 	//printf("external trigger rate: %.02f Hz \n", 1.0/(timeAfterADCInt - oldTimeAfterADCInt));
 	 	 	//triggerRate = ceil(1.0/(timeAfterADCInt - oldTimeAfterADCInt));
 	 	 	triggerRate = 1.0/(timeAfterADCInt - oldTimeAfterADCInt);
            oldTimeAfterADCInt = timeAfterADCInt;
            //time_t t1 = time(NULL);
            timeval tim;
            gettimeofday(&tim, NULL);
            double t1 = tim.tv_sec + (tim.tv_usec/1000000.0);
 /*
 	 	 	epicsTimeToStrftime(buf, 30, "%Y/%m/%d %H:%M:%S.%06f", &startTime);
 	 	 	printf("staTime: %s \n",buf);
*/
			epicsMutexLock(pics710Driver->daqMutex);
/*
 	 	 	epicsTimeGetCurrent(&now);
 	 	 	timeAfterADCInt = now.secPastEpoch + now.nsec/1000000000.0;
*/
			if (0 > (errorCode = read(pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead)))
			{
				printf ("can't read the data, errorCode: %d \n", errorCode);
				epicsMutexUnlock(pics710Driver->daqMutex);
				goto daqStart;
			}
/*
 	 	 	epicsTimeGetCurrent(&now);
 	 	 	timeAfterRead = now.secPastEpoch + now.nsec/1000000000.0;
 	 	 	printf("time spent on raw ADC data readout: %f seconds\n", (timeAfterRead - timeAfterADCInt));
*/
            epicsMutexUnlock(pics710Driver->daqMutex);

 	 	 	epicsTimeGetCurrent(&now);
 	 	 	timeAfterRead = now.secPastEpoch + now.nsec/1000000000.0;
 	 	 	//printf("time spent on raw ADC data readout: %f seconds\n", (timeAfterRead - timeAfterADCInt));
 	 	 	//time_t t2 = time(NULL);
 	 	 	//printf("%f seconds \n",t2 - t1);//time() only gives second resolution
 	 	 	gettimeofday(&tim, NULL);
 	 	 	double t2 = tim.tv_sec + (tim.tv_usec/1000000.0);
 	 	 	//printf("%f seconds \n",t2 - t1);
 	 	 	//if ((timeAfterRead - timeAfterADCInt) < 0.005)
 	 	 	if ((t2 - t1) < 0.005)
 	 	 	{
 	 	 		i++;
 	 	 	 	//printf("%d \n",i);
 	 	 		/*epicsTimeGetCurrent(&now);
 	 	 	 	epicsTimeToStrftime(buf, 30, "%Y/%m/%d %H:%M:%S.%06f", &now);
 	 	 	 	printf("time: %s \n",buf);*/
 	 	 	}
 	 	 	else
 	 	 	{
 	 	 		printf("big jump(%f seconds) occurred at every %dth trigger \n",(timeAfterRead - timeAfterADCInt), i+1);
 	 	 		i = 0;
 	 	 	}

			if (pics710Driver->control.packed_data == 0)
				fileUnpackedData710(pics710Driver);
			else
				filePackedData710(pics710Driver);

            ics710Debug("scanIoRequest: send interrupt to waveform records \n");
            scanIoRequest(pics710Driver->ioscanpvt);
            pics710Driver->count++;
/*
 	 	 	epicsTimeGetCurrent(&now);
 	 	 	epicsTimeToStrftime(buf, 30, "%Y/%m/%d %H:%M:%S.%06f", &now);
 	 	 	//printf("DAQ start: %s \n", buf);
 	 	 	timeAtLoopStart = now.secPastEpoch + now.nsec/1000000000.0;
*/
        } while (pics710Driver->running);

    }//while(1)

  }//void ics710DaqThread(void *arg)

}//extern "C"
