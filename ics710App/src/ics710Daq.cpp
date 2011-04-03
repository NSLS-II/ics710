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
extern double timeAfterADCInt;
extern double triggerRate;
extern double timeAfterRead;
extern double dcOffset[MAX_CHANNEL];

extern "C"
{
  //epicsTimeStamp startTime;
  //static int fileUnpackedData710 (pics710Driver->pAcqData, pics710Driver->totalChannel, i, pics710Driver->nSamples)
  static int fileUnpackedData710 (ics710Driver *pics710Driver)
  {
	  int i;
	  unsigned channel = 0;
	  unsigned nSamples = 0;
	  double rawVolt;
	  ics710Debug("split the unpacked data into different channel data \n");

	  //Trick: get rid of the garbage data: 1024/totalChannel wrong samples at the beginning, garbage occurs again every 32K/totalChannel
	  /*for (nSamples = 0; nSamples < pics710Driver->nSamples; nSamples++)
	  {
		  if (0 == (nSamples % (32000/pics710Driver->totalChannel)))
		  {
			  for (i = 1; i++; i < 256)
			  {
				  if ((nSamples + i) > pics710Driver->nSamples) break;
				  pics710Driver->pAcqData[nSamples + i] = pics710Driver->pAcqData[nSamples];
			  }
		  }
	  }*/
	  //convert the raw data(32-bit long) to voltage, then calibrate it (slope=0.98)
	  for (channel = 0; channel < pics710Driver->totalChannel; channel++)
	  {
		  for (nSamples = 0; nSamples < pics710Driver->nSamples; nSamples++)
		  {
			  if (0 != (channel % 2))
			  {
				  //1<<23 = 8388608, volt-volt/50 (50Ohm?), ch1DcOffset=0.173 for 10V Input Range
				  rawVolt = ((int)pics710Driver->pAcqData[nSamples * pics710Driver->totalChannel + channel - 1] / (256* 8388608.0)) * (10.00 / (1+pics710Driver->gainControl.input_voltage_range));
				  pics710Driver->chData[channel][nSamples] = 0.98 * rawVolt - dcOffset[channel];
				  //pics710Driver->chData[channel][nSamples] = 0.980335 * rawVolt - 0.173;
				  //pics710Driver->chData[channel][nSamples] = rawVolt;
				  //pics710Driver->chData[channel][nSamples] = rawVolt*(1-1/(50*(1+pics710Driver->gainControl.input_voltage_range))) - 0.046;
			  }
			  else
			  {
				  //1<<23 = 8388608, volt-volt/50 (50Ohm?), ch0DcOffset=0.0481
				  rawVolt = ((int)pics710Driver->pAcqData[nSamples * pics710Driver->totalChannel + channel + 1] / (256* 8388608.0)) * (10.00 / (1+pics710Driver->gainControl.input_voltage_range));
				  pics710Driver->chData[channel][nSamples] = 0.98 * rawVolt - dcOffset[channel];
 //pics710Driver->chData[channel][nSamples] = -1.4662e-05*(rawVolt*rawVolt*rawVolt)+1.2958e-04*(rawVolt*rawVolt)+9.8039e-01*rawVolt-4.7776e-02;
				  //p21 = -1.4662e-05   1.2958e-04   9.8039e-01  -4.7776e-02
				  //pics710Driver->chData[channel][nSamples] = 0.980335 * rawVolt - 0.046976;
				  //pics710Driver->chData[channel][nSamples] = rawVolt;
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
     int timeout = 2; /* seconds */
     epicsTimeStamp now;
     double oldTimeAfterADCInt;
     int i = 0;

    while (1)
    {
		ics710Debug("enter ics710DaqThread. \n");
    	epicsThreadSleep(1.00);
    	epicsEventWait(pics710Driver->runSemaphore);
        do
        {
 daqStart:
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
 	 	 	epicsTimeGetCurrent(&now);
 	 	 	timeAfterADCInt = now.secPastEpoch + now.nsec/1000000000.0;
 	 	 	triggerRate = 1.0/(timeAfterADCInt - oldTimeAfterADCInt);
            oldTimeAfterADCInt = timeAfterADCInt;

			//epicsMutexLock(pics710Driver->daqMutex);
			if (0 > (errorCode = read(pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead)))
			{
				printf ("can't read the data, errorCode: %d \n", errorCode);
				//epicsMutexUnlock(pics710Driver->daqMutex);
				goto daqStart;
			}
            //epicsMutexUnlock(pics710Driver->daqMutex);
 	 	 	epicsTimeGetCurrent(&now);
 	 	 	timeAfterRead = now.secPastEpoch + now.nsec/1000000000.0;

			if (pics710Driver->control.packed_data == 0)
				fileUnpackedData710(pics710Driver);
			else
				filePackedData710(pics710Driver);

            ics710Debug("scanIoRequest: send interrupt to waveform records \n");
            scanIoRequest(pics710Driver->ioscanpvt);
            pics710Driver->count++;

        } while (pics710Driver->running);

    }//while(1)

  }//void ics710DaqThread(void *arg)

}//extern "C"
