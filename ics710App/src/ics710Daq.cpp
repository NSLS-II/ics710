/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include "ics710Daq.h"
#include "ics710Drv.h"

#include "ics710api.h"

#include "epicsThread.h"
#include "epicsTime.h"

extern "C"
{
  //static int fileUnpackedData710 (pics710Driver->pAcqData, pics710Driver->totalChannel, i, pics710Driver->nSamples)
  static int fileUnpackedData710 (ics710Driver *pics710Driver)
  {
	  unsigned channel = 0;
	  unsigned nSamples = 0;

	  ics710Debug("split the unpacked data into different channel data \n");
	  for (channel = 0; channel < pics710Driver->totalChannel; channel++)
	  {
		  for (nSamples = 0; nSamples < pics710Driver->nSamples; nSamples++)
		  {
			  if (0 != (channel % 2))
				  pics710Driver->chData[channel][nSamples]  =
				   		  (((int)pics710Driver->pAcqData[nSamples * pics710Driver->totalChannel + channel - 1] / (256* 8388608.0))
				           * (10.00 / (1+pics710Driver->gainControl.input_voltage_range)) * 0.98) - 0.046; //1<<23 = 8388608, volt-volt/50 (50Ohm?), ch1DcOffset=0.0481
			  else
				  pics710Driver->chData[channel][nSamples] =
						  (((int)pics710Driver->pAcqData[nSamples * pics710Driver->totalChannel + channel + 1] / (256* 8388608.0))
				           * (10.00 / (1+pics710Driver->gainControl.input_voltage_range)) * 0.98) -0.046;

			  //(pics710Driver->chData[channel])++;
		  }//		  for (nSamples = 0; nSamples < pics710Driver->nSamples; nSamples++)

	  }//	  for (channel = 0; channel < pics710Driver->totalChannel; channel++)

	  ics710Debug("channel #1 data from 600 to 610: \n");
	  for (nSamples = 600; nSamples < 610; nSamples++)
		  ics710Debug("%f \t", pics710Driver->chData[0][nSamples]);

	  ics710Debug("channel #2 data from 600 to 610: \n");
	  for (nSamples = 600; nSamples < 610; nSamples++)
		  ics710Debug("%f \t", pics710Driver->chData[1][nSamples]);

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
     pics710Driver->running = 1;

    while (1)
    {
		ics710Debug("enter ics710DaqThread. \n");
    	epicsThreadSleep(1.00);
    	//epicsEventWait(pics710Driver->runSemaphore);
		//printf (" get runSemaphore \n");
        do
        {
 daqStart:
			if (ICS710_OK != (errorCode = ics710BufferReset (pics710Driver->hDevice)))
			{
				printf("can't reset buffer register, errorCode: %d \n", errorCode);
				goto daqStart;
			}

			if (ICS710_OK != (errorCode = ics710WaitADCInt(pics710Driver->hDevice, &timeout)))
			{
				printf ("wait ADC interrupt timeout(%d seconds), errorCode: %d \n", timeout, errorCode);
				pics710Driver->timeouts++;
				goto daqStart;
			}
			//epicsMutexLock(pics710Driver->daqMutex);
			//epicsMutexLock(ics710DmaMutex);
			//if (ICS710_OK != (errorCode = read(pics710Driver->hDevice, pics710Driver->pAcqData + i*(pics710Driver->bufLength +1)*2, pics710Driver->bytesToRead)))
			//'cast long' still works: bufLength = 500 - 1, pics710Driver->pAcqData + 2*500 =  1000 * 4 (4Bytes/long) = 4K; must use i*()*2, if use i*()*3, errorCode=-1
			if (0 > (errorCode = read(pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead)))
			{
				printf ("can't read the data, errorCode: %d \n", errorCode);
				goto daqStart;
			}
            //epicsMutexUnlock(ics710DmaMutex);
            //epicsMutexUnlock(pics710Driver->daqMutex);

			if (pics710Driver->control.packed_data == 0)
				fileUnpackedData710(pics710Driver);
			else
				filePackedData710(pics710Driver);

            scanIoRequest(pics710Driver->ioscanpvt);
            ics710Debug("scanIoRequest: send interrupt to waveform records \n");
            pics710Driver->count++;

        } while (pics710Driver->running);

    }//while(1)

  }//void ics710DaqThread(void *arg)

}//extern "C"
