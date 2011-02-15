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
     int timeout = 1; /* seconds */
     pics710Driver->running = 1;
     epicsTimeStamp now;

    while (1)
    {
		printf ("enter ics710DaqThread. \n");
    	//epicsEventWait(pics710Driver->runSemaphore);
		//printf (" get runSemaphore \n");
        do
        {
    		if ( 0 != ics710Config(pics710Driver) )
    		{
    			printf ("can't reconfigure ics710. \n");
    		}

    		if(ICS710_TRIG_INTERNAL == pics710Driver->control.trigger_select)
    		{
    			if (ics710Trigger (pics710Driver->hDevice) != ICS710_OK)
    				printf ("ics710Trigger error! \n");
    		}

        	if (ics710WaitADCInt (pics710Driver->hDevice, &timeout) != ICS710_OK)
        	{
        		printf ("ics710WaitADCInt error! \n");
        		pics710Driver->timeouts++;
        	}

			epicsMutexLock(pics710Driver->daqMutex);
            epicsMutexLock(ics710DmaMutex);
        	if (0 > read (pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead))
        	{
        		printf ("Error: Read failed\n");
        	}

            epicsMutexUnlock(ics710DmaMutex);
            epicsMutexUnlock(pics710Driver->daqMutex);
            scanIoRequest(pics710Driver->ioscanpvt);
            pics710Driver->count++;

        	printf ("read %d bytes raw data using DMA at the time: \n", pics710Driver->bytesToRead);
            epicsTimeGetCurrent(&now);
            epicsTimeShow(&now, 0);
        	printf ("the 2001th data is: %f \n", 10.00 * (int)pics710Driver->pAcqData[2001] / (256* 8388608.00) );
        	//inputRange * (int)pRawdata[i * totalChannel + channel - 1] / (256* 8388608.00)

        	//epicsThreadSleep(1.00);

        } while (pics710Driver->running);

    }//while(1)

  }//void ics710DaqThread(void *arg)

}//extern "C"
