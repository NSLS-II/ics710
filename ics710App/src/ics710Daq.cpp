/*Yong Hu: 02-08-2010*/

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
				   		  ((int)pics710Driver->pAcqData[nSamples * pics710Driver->totalChannel + channel - 1] / (256* 8388608.0))
				           * (10.00 / (1+pics710Driver->gainControl.input_voltage_range)); //1<<23 = 8388608
			  else
				  pics710Driver->chData[channel][nSamples] =
						  ((int)pics710Driver->pAcqData[nSamples * pics710Driver->totalChannel + channel + 1] / (256* 8388608.0))
				           * (10.00 / (1+pics710Driver->gainControl.input_voltage_range));

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
     int timeout = 1; /* seconds */
     pics710Driver->running = 1;
     epicsTimeStamp now;
     unsigned i;

    while (1)
    {
		ics710Debug("enter ics710DaqThread. \n");
    	epicsThreadSleep(1.00);
    	//epicsEventWait(pics710Driver->runSemaphore);
		//printf (" get runSemaphore \n");
        do
        {
/*			if (ICS710_TRIG_INTERNAL == pics710Driver->control.trigger_select) //don't put triggering here
    		{
    			if (ICS710_OK != (errorCode = ics710Trigger(pics710Driver->hDevice)))
    				printf ("can't software trigger the board, errorCode: %d \n", errorCode);
    			//break;
    		}
*/
 daqStart:
			if(ICS710_OK != (errorCode = ics710StatusGet(pics710Driver->hDevice, &pics710Driver->stat)))
			{
				printf("can't read status register, errorCode: %d \n", errorCode);
			}
			else
			{
				printf("status at the loop beginning: adcIntReq = %llu, pciBufOflow = %llu, irq = %llu, triggered = %llu \n",
						pics710Driver->stat.adc_intrpt_rqst, pics710Driver->stat.pci_buf_oflow,
						pics710Driver->stat.irq, pics710Driver->stat.board_triggered);
			}

			for (i = 0; i < pics710Driver->swapTimes; i++)//swapTimes=2 works for Continuous Mode, seems not for Capture mode;
			//for(i = 0; i < 2; i++)//swapTimes=2 works for Continuous Mode, seems not for Capture mode;
			{
				if (ICS710_OK != (errorCode = ics710Enable (pics710Driver->hDevice)))
				{
					printf("can't enable the board, errorCode: %d \n", errorCode);
	    			goto daqStart;
				}

				if (ICS710_CAPTURE_WITHPRETRG == pics710Driver->control.acq_mode)
				{
					if (ICS710_OK != (errorCode = (ics710Arm (pics710Driver->hDevice))))
					{
						printf ("can't arm the board, errorCode: %d \n", errorCode);
		    			goto daqStart;
					}
				}

	    		if (ICS710_TRIG_INTERNAL == pics710Driver->control.trigger_select) //for Continuous Mode, either put here or above works
	    		{
	    			if (ICS710_OK != (errorCode = ics710Trigger(pics710Driver->hDevice)))
	    				printf ("can't software trigger the board, errorCode: %d \n", errorCode);
	    			//break;
	    			goto daqStart;
	    		}

	        	if (ICS710_OK != (errorCode = ics710WaitADCInt(pics710Driver->hDevice, &timeout)))
	        	{
					printf ("wait ADC interrupt timeout(%d seconds), errorCode: %d \n", timeout, errorCode);
	        		pics710Driver->timeouts++;
	        		//break;
	    			goto daqStart;
	        	}

				if(ICS710_OK != (errorCode = ics710StatusGet(pics710Driver->hDevice, &pics710Driver->stat)))
				{
					printf("can't read status register, errorCode: %d \n", errorCode);
				}
				else
				{
					printf("status after ADC interrupt #%d: adcIntReq = %llu, pciBufOflow = %llu, irq = %llu, triggered = %llu \n",
						i,  pics710Driver->stat.adc_intrpt_rqst, pics710Driver->stat.pci_buf_oflow,
							pics710Driver->stat.irq, pics710Driver->stat.board_triggered);
				}
/*
	    		if (ICS710_TRIG_INTERNAL == pics710Driver->control.trigger_select) //for Continuous Mode, either put here or above works
	    		{
	    			if (ICS710_OK != (errorCode = ics710Trigger(pics710Driver->hDevice)))
	    				printf ("can't software trigger the board, errorCode: %d \n", errorCode);
	    			//break;
	    		}
*/
			    //epicsMutexLock(pics710Driver->daqMutex);
                //epicsMutexLock(ics710DmaMutex);
	        	//if (ICS710_OK != (errorCode = read(pics710Driver->hDevice, pics710Driver->pAcqData + i*(pics710Driver->bufLength +1)*2, pics710Driver->bytesToRead)))
				//'cast long' still works: bufLength = 500 - 1, pics710Driver->pAcqData + 2*500 =  1000 * 4 (4Bytes/long) = 4K; must use i*()*2, if use i*()*3, errorCode=-1
				if (0 > (errorCode = read(pics710Driver->hDevice, pics710Driver->pAcqData + i*(pics710Driver->bufLength +1)*2, pics710Driver->bytesToRead)))
			    {
	        		printf ("can't read the data, errorCode: %d \n", errorCode);
	        		//break;
	    			goto daqStart;
	        	}

				if(ICS710_OK != (errorCode = ics710StatusGet(pics710Driver->hDevice, &pics710Driver->stat)))
				{
					printf("can't read status register, errorCode: %d \n", errorCode);
				}
				else
				{
					printf("status after reading the data #%d: adcIntReq = %llu, pciBufOflow = %llu, irq = %llu, triggered = %llu\n",
						i,	pics710Driver->stat.adc_intrpt_rqst, pics710Driver->stat.pci_buf_oflow,
							pics710Driver->stat.irq, pics710Driver->stat.board_triggered);
				}


	        	if (ICS710_OK != (errorCode = ics710Disable(pics710Driver->hDevice))) // If disable the acquisition, must re-enable at the beginning of the loop
	        	{
	        		printf ("can't disable the acquisition, errorCode: %d \n", errorCode);
	        		//break;
	    			goto daqStart;
	        	}

			}//for(i = 0; i < pics710Driver->swapTimes; i++)
/*
        	if (ICS710_OK != (errorCode = ics710Disable(pics710Driver->hDevice))) // If disable the acquisition, even Continuous Mode won't work;
        	{
        		printf ("can't disable the acquisition, errorCode: %d \n", errorCode);
        		//break;
        	}
*/
            //epicsMutexUnlock(ics710DmaMutex);
            //epicsMutexUnlock(pics710Driver->daqMutex);

			if (pics710Driver->control.packed_data == 0)
				fileUnpackedData710(pics710Driver);
			else
				filePackedData710(pics710Driver);
/*
			// 02-21-2011: reconfigure the board: only reach 2Hz(20KS/s, 1K samples/ch, 2-ch)
			if (0 != ics710Config(pics710Driver))
			{
				printf("Error: can't reconfigure the board \n");
				//return -1;
			}
*/
            scanIoRequest(pics710Driver->ioscanpvt);
            ics710Debug("scanIoRequest: send interrupt to waveform records \n");
            pics710Driver->count++;
/*
        	printf ("read %d bytes raw data using DMA at the time: \n",pics710Driver->swapTimes *  pics710Driver->bytesToRead);
            epicsTimeGetCurrent(&now);
            epicsTimeShow(&now, 0);
        	printf ("the 800th data is: %f \n\n\n", 10.00 * (int)pics710Driver->pAcqData[800] / (256* 8388608.00) );
*/
        	//epicsThreadSleep(1.00);

        } while (pics710Driver->running);

    }//while(1)

  }//void ics710DaqThread(void *arg)

}//extern "C"
