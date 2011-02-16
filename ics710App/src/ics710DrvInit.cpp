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

#include "ics710Drv.h"
#include "ics710Daq.h"

#include "ics710api.h"

#include <epicsExport.h>
#include <iocsh.h>
#include "epicsThread.h"

extern "C"
{
  ics710Driver ics710Drivers[MAX_DEV];
  //unsigned nbrIcs710Drivers = 0;
  epicsMutexId ics710DmaMutex;

  static int releaseBoard(ics710Driver *pics710Driver)
  {
	  if ( (0 < pics710Driver->hDevice) && (NULL != pics710Driver->pAcqData) )
	  {
		    printf ("release resources \n");
			ics710FreeDmaBuffer (pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead);
			pics710Driver->pAcqData = NULL;
			close(pics710Driver->hDevice);
			pics710Driver->hDevice = -1;

	  }

	return 0;
  }

  int ics710Reconfig(ics710Driver *pics710Driver)
  {
/*	  unsigned long long dmaLocalSpace = 0;
	  time_t   strobe;

		if (ics710BoardReset (pics710Driver->hDevice) != ICS710_OK)
			errorExit(BOARD_RESET_ERROR, pics710Driver);

		if (ics710ControlSet (pics710Driver->hDevice, &(pics710Driver->control)) != ICS710_OK)
			errorExit(CONTROL_ERROR, pics710Driver);

		if (ics710ADCClockSet (pics710Driver->hDevice, &pics710Driver->ics710AdcClockRate, &pics710Driver->actualADCRate) != ICS710_OK)
			errorExit(ADC_CLOCK_ERROR, pics710Driver);

		if (ics710MasterControlSet (pics710Driver->hDevice, &(pics710Driver->masterControl)) != ICS710_OK)
			errorExit(MASTER_CONTROL_ERROR, pics710Driver);

		if (ics710ChannelCountSet (pics710Driver->hDevice, &(pics710Driver->channelCount)) != ICS710_OK)
			errorExit(CHANNEL_COUNTL_ERROR, pics710Driver);
			//printf ("wrong channel count \n");
		//else printf (" channel count: %d\n", pics710Driver->channelCount);

		if (ics710BufferLengthSet (pics710Driver->hDevice, &(pics710Driver->bufLength)) != ICS710_OK)
			errorExit(BUFFER_LENGTH_ERROR, pics710Driver);

		if (ics710AcquireCountSet (pics710Driver->hDevice, &(pics710Driver->acqLength)) != ICS710_OK)
			errorExit(ACQUIRE_COUNT_ERROR, pics710Driver);

		if (ics710DmaLocalSpaceSet (pics710Driver->hDevice, &dmaLocalSpace) != ICS710_OK)
			errorExit(DMA_LOCAL_SPACE_ERROR, pics710Driver);

		// Buffer Reset to load the new register values
		if (ics710BufferReset (pics710Driver->hDevice) != ICS710_OK)
			errorExit(BUFFER_RESET_ERROR, pics710Driver);

		if (ics710ADCReset (pics710Driver->hDevice) != ICS710_OK)
			errorExit(ADC_RESET_ERROR, pics710Driver);
		strobe = time(NULL);
		do
		{
			usleep(0);
			if(ics710StatusGet(pics710Driver->hDevice, &pics710Driver->stat) != ICS710_OK)
				errorExit(STATUS_ERROR, pics710Driver);

			if((time(NULL) - strobe) > 1.0)
				errorExit(TIMEOUT_ERROR, pics710Driver);

		} while(0 != pics710Driver->stat.cal);

		if (ics710Enable (pics710Driver->hDevice) != ICS710_OK)
			errorExit(ENABLE_ERROR, pics710Driver);
*/
		return 0;

  }

  int ics710Config(ics710Driver *pics710Driver)
  {
	  int errorCode = 0;
	  double defaultADCFreq;
	  double defaultFPDPFreq;
	  double ics710FpdpRate = 30;
	  double actualFPDPRate;
	  unsigned long long fifoFrames = 1;
	  unsigned long long dmaLocalSpace = 0;
	  time_t   strobe;
	  int timeout = 5;

		if (ICS710_OK != (errorCode = ics710BoardReset (pics710Driver->hDevice)))
		{
			printf("can't reset board, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		if (ICS710_OK != (errorCode = ics710ADCFPDPDefaultClockSet (pics710Driver->hDevice, &defaultADCFreq, &defaultFPDPFreq)))
		{
			printf("can't set default ADC&FPDP clocks, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		if (ICS710_OK != (errorCode = ics710FPDPClockSet (pics710Driver->hDevice, &ics710FpdpRate, &actualFPDPRate)))
		{
			printf("can't set FPDP clock, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		if (ICS710_OK != (errorCode = ics710ADCClockSet (pics710Driver->hDevice, &pics710Driver->ics710AdcClockRate, &pics710Driver->actualADCRate)))
		{
			printf("can't set ADC clock, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}
		 //printf ("wrong ADC clock rate \n");
	    //else printf (" ADC clock rate: %f\n", pics710Driver->actualADCRate);

		if (ICS710_OK != (errorCode = ics710ControlSet (pics710Driver->hDevice, &(pics710Driver->control))))
		{
			printf("can't set control register, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		if (ICS710_OK != (errorCode = ics710MasterControlSet (pics710Driver->hDevice, &(pics710Driver->masterControl))))
		{
			printf("can't set master control register, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		if (ICS710_OK != (errorCode = ics710GainSet (pics710Driver->hDevice, &(pics710Driver->gainControl))))
		{
			printf("can't set gain control register, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		if (ICS710_OK != (errorCode = ics710FilterSet (pics710Driver->hDevice, &(pics710Driver->filterControl))))
		{
			printf("can't set filter control register, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		if (ICS710_OK != (errorCode = ics710ChannelCountSet (pics710Driver->hDevice, &(pics710Driver->channelCount))))
		{
			printf("can't set channel count register, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}
	    //printf ("wrong channel count \n");
		//else printf (" channel count: %d\n", pics710Driver->channelCount);

		if (ICS710_OK != (errorCode = ics710BufferLengthSet (pics710Driver->hDevice, &(pics710Driver->bufLength))))
		{
			printf("can't set buffer length register, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		if (ICS710_OK != (errorCode = ics710AcquireCountSet (pics710Driver->hDevice, &(pics710Driver->acqLength))))
		{
			printf("can't set acquisition length register, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		if (ICS710_OK != (errorCode = ics710FPDPFramesSet (pics710Driver->hDevice, &(fifoFrames))))
		{
			printf("can't set FPDP frame register, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		if (ICS710_OK != (errorCode = ics710DmaLocalSpaceSet (pics710Driver->hDevice, &dmaLocalSpace)))
		{
			printf("can't set DMA local space, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		/* Buffer Reset to load the new register values    */
		if (ICS710_OK != (errorCode = ics710BufferReset (pics710Driver->hDevice)))
		{
			printf("can't reset buffer register, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		// ADC Reset and begin Zero Calibration: must do this
		if (ICS710_OK != (errorCode = ics710ADCReset (pics710Driver->hDevice)))
		{
			printf("can't reset ADC, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}
		// ADC sync
		strobe = time(NULL);
		do
		{
			usleep(0);
			if(ICS710_OK != (errorCode = ics710StatusGet(pics710Driver->hDevice, &pics710Driver->stat)))
			{
				printf("can't read status register, errorCode: %d \n", errorCode);
				releaseBoard(pics710Driver);
				break;
			}
			if((time(NULL) - strobe) > timeout)
			{
				printf("ADC calibration timeout: waited for %d seconds \n", timeout);
				releaseBoard(pics710Driver);
				break;
			}

		} while(0 != pics710Driver->stat.cal);

		if (ICS710_OK != (errorCode = ics710Enable (pics710Driver->hDevice)))
		{
			printf("can't enable the board, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		return errorCode;

  }// int ics710Config(ics710Driver *pics710Driver)

  static int ics710Init(int card_totalChannel_nSamples_gain_filter_adcClockRate_triggerSel)
  {
	  unsigned card = 1; /*card: No.1 ~ No.8*/
	  unsigned totalChannel = 2;
	  unsigned nSamples  = 10000;
	  unsigned gain = 0;
	  unsigned filter = 1;
	  unsigned adcClockRate = 5.12; /*5.12 Mhz --> 20KHz data rate*/
	  unsigned triggerSel = 1; /*0: internal; 1: external triggering*/
      char name[32];
	  //double inputRange;
	  //int timeout = 5; /* 5 sec timeout */
	  //unsigned short i = 0;

   // for(module=0; module < MAX_DEV; )
	//for(module=0; module < 1; )
	//{
		ics710Driver *pics710Driver = &ics710Drivers[card];
		pics710Driver->hDevice = -1;
		//pics710_driver->hDevice = open("/dev/ics710-%u", O_RDWR, module+1);//doesn't work
		switch(card)
		{
		case 1:
			{
				pics710Driver->hDevice = open("/dev/ics710-1", O_RDWR);
				break;
			}
		case 2:
			{
				pics710Driver->hDevice = open("/dev/ics710-2", O_RDWR);
				break;
			}
		case 3:
			{
				pics710Driver->hDevice = open("/dev/ics710-3", O_RDWR);
				break;
			}
		case 4:
			{
				pics710Driver->hDevice = open("/dev/ics710-4", O_RDWR);
				break;
			}
		case 5:
			{
				pics710Driver->hDevice = open("/dev/ics710-5", O_RDWR);
				break;
			}
		case 6:
			{
				pics710Driver->hDevice = open("/dev/ics710-6", O_RDWR);
				break;
			}
		case 7:
			{
				pics710Driver->hDevice = open("/dev/ics710-7", O_RDWR);
				break;
			}
		case 8:
			{
				pics710Driver->hDevice = open("/dev/ics710-8", O_RDWR);
				break;
			}
		default:
			break;
		} //switch(card)

		if (0 > pics710Driver->hDevice)
		{
			printf ("Error: cannot open a handle for /dev/ics710-%u, check out if ics710.ko is loaded and /dev/ics710[1:8] exists \n",card+1);
			return -1;
		}
		else
		{
			/* blocking I/O: not efficient? */
			fcntl(pics710Driver->hDevice, F_SETFL, fcntl(pics710Driver->hDevice, F_GETFL) & ~O_NONBLOCK);

		/* default parameters */
			pics710Driver->control.adc_clock_select = ICS710_CLOCK_INTERNAL;/*ICS710_CLOCK_INTERNAL or ICS710_CLOCK_EXTERNAL*/
			pics710Driver->control.diag_mode_enable = ICS710_DISABLE; /* ICS710_ENABLE or ICS710_DISABLE */
			pics710Driver->control.fpdp_enable  = ICS710_DISABLE;/* ICS710_ENABLE or ICS710_DISABLE */
			pics710Driver->control.enable = ICS710_DISABLE;/* ICS710_ENABLE or ICS710_DISABLE */
			pics710Driver->control.int_trigger = ICS710_INACTIVE; /* ICS710_INACTIVE or ICS710_ACTIVE*/
			pics710Driver->control.oversamp_ratio = ICS710_SAMP_NORMAL; /*0:ICS710_SAMP_NORMAL,1:ICS710_SAMP_DOUBLE,2:ICS710_SAMP_QUAD*/
			pics710Driver->control.adc_hpfilter_enable = ICS710_DISABLE; /* ICS710_ENABLE or ICS710_DISABLE                  */
			pics710Driver->control.zero_cal = ICS710_ZCAL_INTERNAL;/* 0: ICS710_ZCAL_EXTERNAL, 1:ICS710_ZCAL_INTERNAL */
			pics710Driver->control.packed_data = ICS710_UNPACKED_DATA;/* ICS710_UNPACKED_DATA or ICS710_PACKED_DATA */
			pics710Driver->control.acq_mode = ICS710_CONTINUOUS; /* ICS710_CONTINUOUS or ICS710_CAPTURE_NOPRETRG or ICS710_CAPTURE_WITHPRETRG  */
			pics710Driver->control.system_master = ICS710_ENABLE; /* ICS710_ENABLE or ICS710_DISABLE  */
			pics710Driver->control.adc_master = ICS710_ENABLE; /* ICS710_ENABLE or ICS710_DISABLE */
			pics710Driver->control.fpdp_master = ICS710_ENABLE; /* ICS710_ENABLE or ICS710_DISABLE */
			pics710Driver->control.adc_termin = ICS710_ENABLE; /* ICS710_ENABLE or ICS710_DISABLE */
			pics710Driver->control.fpdp_termin = ICS710_ENABLE; /* ICS710_ENABLE or ICS710_DISABLE */
			pics710Driver->control.extclk_term = ICS710_ENABLE; /* ICS710_ENABLE or ICS710_DISABLE*/
			pics710Driver->control.extrig_term = ICS710_ENABLE;/* ICS710_ENABLE or ICS710_DISABLE */
			pics710Driver->control.extrig_mode = ICS710_EXTRIG_RISING;/* 0:ICS710_EXTRIG_HIGH,1:ICS710_EXTRIG_RISING,2:ICS710_EXTRIG_LOW, 3: ICS710_EXTRIG_FALLING*/
			pics710Driver->masterControl.board_address = 0;
		/*setup parameters during initialization */
			pics710Driver->control.trigger_select = triggerSel; /*ICS710_TRIG_INTERNAL or ICS710_TRIG_EXTERNAL*/
			pics710Driver->gainControl.input_voltage_range = gain;
			//inputRange = 10.0/(1+gainControl.input_voltage_range);
			pics710Driver->filterControl.cutoff_freq_range = filter;
			pics710Driver->totalChannel = totalChannel;
			pics710Driver->nSamples = nSamples;
			pics710Driver->ics710AdcClockRate = adcClockRate; /*5.12MHz*/

			/* Calculated parameters: acquisition length, channel count, buffer length*/
			pics710Driver->acqLength = pics710Driver->nSamples - 1;

			if (pics710Driver->control.packed_data == 0)
				pics710Driver->channelCount = pics710Driver->totalChannel - 1; /* Channel Count Register, for un-packed data */
			else
				pics710Driver->channelCount = pics710Driver->totalChannel / 2 - 1; /* Channel Count Register, for packed data */
			pics710Driver->masterControl.numbers_of_channels = pics710Driver->channelCount;/* For Single Board, the two always equal */

			if (pics710Driver->control.packed_data == 0)
				pics710Driver->bufLength = (pics710Driver->totalChannel * pics710Driver->nSamples / 2)/2 - 1;
			else
				pics710Driver->bufLength = (pics710Driver->totalChannel * pics710Driver->nSamples / 4)/2 - 1;
			if (pics710Driver->bufLength > 262143)
			{
				printf ("Error: Calculated buf_len > 256K(4 MBytes memory board).\n");
				return -1;
			}

			/*allocate DMA memory and clear it*/
			pics710Driver->bytesToRead = (pics710Driver->bufLength + 1) * 8;  /* bytes to be read from ics-710 memory*/
			pics710Driver->pAcqData = (long *) ics710AllocateDmaBuffer (pics710Driver->hDevice, 2 * pics710Driver->bytesToRead);
			if (NULL == pics710Driver->pAcqData)
			{
				printf ("Error: Not enough memory for %u bytes\n", pics710Driver->bytesToRead);
				return -1;
			}
			memset (pics710Driver->pAcqData, 0, 2 * pics710Driver->bytesToRead);

			/* configure the board */
			if (0 != ics710Config(pics710Driver))
			{
				printf("Error: can't configure the board \n");
				return -1;
			}

			/*debugging*/
			printf ("%d channels; %d samples per channel \n", pics710Driver->totalChannel, pics710Driver->nSamples);
			if(0 == pics710Driver->control.trigger_select)
				printf("internal triggering\n");
			else
				printf("external triggering \n");
			if (pics710Driver->control.oversamp_ratio == 0)
				printf ("Normal Sampling Speed, data output rate = %3.3f kHz \n", 1000 * pics710Driver->actualADCRate / 256);
			else if (pics710Driver->control.oversamp_ratio == 1)
				printf ("Double Sampling Speed, data output rate = %3.3f kHz \n", 1000 * pics710Driver->actualADCRate / 128);
			else if (pics710Driver->control.oversamp_ratio == 2)
				printf ("Quad Sampling Speed, data output rate = %3.3f kHz \n", 1000 * pics710Driver->actualADCRate / 64);
			printf ("Input Voltage Range: %2.3f (V) \n", 10.0/(1+pics710Driver->gainControl.input_voltage_range));
			printf ("Cut-off Frequency = %d kHz \n", pics710Driver->filterControl.cutoff_freq_range * 10);

			/*create a thread*/
			pics710Driver->runSemaphore = epicsEventMustCreate(epicsEventEmpty);
			pics710Driver->daqMutex = epicsMutexMustCreate();
			ics710DmaMutex = epicsMutexMustCreate();
			pics710Driver->count = 0;
			snprintf(name, sizeof(name), "tics710Daq%u", card);
			scanIoInit(&pics710Driver->ioscanpvt);
			epicsThreadMustCreate(name,epicsThreadPriorityMin,5000000,ics710DaqThread,pics710Driver);
			printf("spawn a data acquisition thread for card #%d: %s \n ", card, name);
		}//else if (0 > pics710Driver->hDevice)

	//} //for(module=0; module < MAX_DEV; )

	printf("ics710 card #%d is initialized\n", card);
	return 0;
  }//  static int ics710Init(int card_totalChannel_nSamples_gain_filter_adcClockRate_triggerSel)

  static const iocshArg ics710InitArg0 = {"nSamples",iocshArgInt};
  static const iocshArg * const ics710InitArgs[1] = {&ics710InitArg0};
  static const iocshFuncDef ics710InitFuncDef =
    {"ics710Init",1,ics710InitArgs};

  static void ics710InitCallFunc(const iocshArgBuf *arg)
  {
	  ics710Init(arg[0].ival);
  }

  void ics710Registrar()
  {
	  iocshRegister(&ics710InitFuncDef,ics710InitCallFunc);
  }

  epicsExportRegistrar(ics710Registrar);
}
