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
  void ics710DaqThread(void *arg)
  {
	 ics710Driver *pics710Driver = reinterpret_cast<ics710Driver*>(arg);
	 int errorCode;
     int timeout = 5; /* seconds */
     pics710Driver->running = 1;
     epicsTimeStamp now;
     int i;

    while (1)
    {
		printf ("enter ics710DaqThread. \n");
    	epicsThreadSleep(1.00);
    	//epicsEventWait(pics710Driver->runSemaphore);
		//printf (" get runSemaphore \n");
        do
        {
  /*  		if ( 0 != (errorCode = ics710Config(pics710Driver)) )
    		{
    			printf ("can't reconfigure ics710, errorCode: %d \n", errorCode);
    		}
*/
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

    		if(ICS710_TRIG_INTERNAL == pics710Driver->control.trigger_select)
    		{
    			if (ICS710_OK != (errorCode = ics710Trigger (pics710Driver->hDevice)))
    				printf ("can't software trigger the board, errorCode: %d \n", errorCode);
    			//break;
    		}

			for(i=0; i<2; i++)//buffer swap: this works for Continuous Mode, not for Capture mode;
			{
	        	if (ICS710_OK != (errorCode = ics710WaitADCInt (pics710Driver->hDevice, &timeout)))
	        	{
					printf ("wait ADC interrupt timeout(%d seconds), errorCode: %d \n", timeout, errorCode);
	        		pics710Driver->timeouts++;
	        		//break;
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

	        	if (0 > (errorCode = read(pics710Driver->hDevice, pics710Driver->pAcqData +
	        			i*(pics710Driver->bufLength +1)*2, pics710Driver->bytesToRead)))
	        	{
	        		printf ("can't read the data, errorCode: %d \n", errorCode);
	        		//break;
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

			}
/*
        	if (ICS710_OK != (errorCode = ics710WaitADCInt (pics710Driver->hDevice, &timeout)))
        	{
				printf ("wait ADC interrupt timeout(%d seconds), errorCode: %d \n", timeout, errorCode);
        		pics710Driver->timeouts++;
        		//break;
        	}

			if(ICS710_OK != (errorCode = ics710StatusGet(pics710Driver->hDevice, &pics710Driver->stat)))
			{
				printf("can't read status register, errorCode: %d \n", errorCode);
			}
			else
			{
				printf("status after ADC interrupt: adcIntReq = %llu, pciBufOflow = %llu, irq = %llu, triggered = %llu \n",
						pics710Driver->stat.adc_intrpt_rqst, pics710Driver->stat.pci_buf_oflow,
						pics710Driver->stat.irq, pics710Driver->stat.board_triggered);
			}

			//epicsMutexLock(pics710Driver->daqMutex);
            //epicsMutexLock(ics710DmaMutex);
        	if (0 > (errorCode = read (pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead)))
        	{
        		printf ("can't read the data, errorCode: %d \n", errorCode);
        		//break;
        	}

			if(ICS710_OK != (errorCode = ics710StatusGet(pics710Driver->hDevice, &pics710Driver->stat)))
			{
				printf("can't read status register, errorCode: %d \n", errorCode);
			}
			else
			{
				printf("status after reading the data: adcIntReq = %llu, pciBufOflow = %llu, irq = %llu, triggered = %llu\n",
						pics710Driver->stat.adc_intrpt_rqst, pics710Driver->stat.pci_buf_oflow,
						pics710Driver->stat.irq, pics710Driver->stat.board_triggered);
			}
*/
/*
	     	if (ICS710_OK !=(errorCode = ics710Disable (pics710Driver->hDevice))) // don't use it as Manual says
	        {
	        	printf ("can't disable DAQ, errorCode: %d \n", errorCode);
	        	//break;
	        }

			if(ICS710_OK != (errorCode = ics710StatusGet(pics710Driver->hDevice, &pics710Driver->stat)))
			{
				printf("can't read status register, errorCode: %d \n", errorCode);
			}
			else
			{
				printf("status after disable DAQ: adcIntReq = %llu, pciBufOflow = %llu, irq = %llu, triggered = %llu\n",
						pics710Driver->stat.adc_intrpt_rqst, pics710Driver->stat.pci_buf_oflow,
						pics710Driver->stat.irq, pics710Driver->stat.board_triggered);
			}
*/
            //epicsMutexUnlock(ics710DmaMutex);
            //epicsMutexUnlock(pics710Driver->daqMutex);
            scanIoRequest(pics710Driver->ioscanpvt);
            pics710Driver->count++;

        	printf ("read %d bytes raw data using DMA at the time: \n", pics710Driver->bytesToRead);
            epicsTimeGetCurrent(&now);
            epicsTimeShow(&now, 0);
        	printf ("the 2001th data is: %f \n\n\n", 10.00 * (int)pics710Driver->pAcqData[2001] / (256* 8388608.00) );
        	//inputRange * (int)pRawdata[i * totalChannel + channel - 1] / (256* 8388608.00)

        	//epicsThreadSleep(1.00);

        } while (pics710Driver->running);

    }//while(1)

  }//void ics710DaqThread(void *arg)

}//extern "C"
