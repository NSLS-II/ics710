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

/*vendor's Device Driver*/
#include "ics710api.h"

#include "epicsExport.h"
#include "iocsh.h"
#include "epicsThread.h"

#include "ics710Drv.h"
#include "ics710Daq.h"

extern "C"
{
  ics710Driver ics710Drivers[MAX_DEV];
  int ics710Initialized = 0;
  //unsigned nbrIcs710Drivers = 0;
  //epicsMutexId ics710DmaMutex;

  static int releaseBoard(ics710Driver *pics710Driver)
  {
	  if ( (0 < pics710Driver->hDevice) && (NULL != pics710Driver->pAcqData) )
	  {
		    printf ("release the board \n");
			ics710FreeDmaBuffer (pics710Driver->hDevice, pics710Driver->pAcqData, pics710Driver->bytesToRead);
			pics710Driver->pAcqData = NULL;
			close(pics710Driver->hDevice);
			pics710Driver->hDevice = -1;

	  }

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

		// ADC Reset and begin Zero Calibration: 02-21-2011: must do ADC reset after board reset
		if (ICS710_OK != (errorCode = ics710ADCReset (pics710Driver->hDevice)))
		{
			printf("can't reset ADC, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}
		strobe = time(NULL);
		do
		{
			usleep(10000);//sleep 10ms
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

		if (ICS710_CAPTURE_WITHPRETRG == pics710Driver->control.acq_mode)
		{
			if (ICS710_OK != (errorCode = (ics710Arm (pics710Driver->hDevice))))
			{
				printf ("can't arm the board, errorCode: %d \n", errorCode);
				releaseBoard(pics710Driver);
				return errorCode;
			}
			epicsThreadSleep(2.00);
		}

		if (ICS710_TRIG_INTERNAL == pics710Driver->control.trigger_select) //for Continuous Mode, either put here or above works
		{
			if (ICS710_OK != (errorCode = ics710Trigger(pics710Driver->hDevice)))
				printf ("can't software trigger the board, errorCode: %d \n", errorCode);
			releaseBoard(pics710Driver);
			return errorCode;
		}

		return errorCode;

  }// int ics710Config(ics710Driver *pics710Driver)

  static int ics710Init(int card, int totalChannel, int nSamples, int gain, int filter,
		  double adcClockRate, int triggerSel, int acqMode)
  {
      char name[32];

      if ((card < 0 || card > 7) || (totalChannel <0 || totalChannel > 32 || (0 != (totalChannel % 2))) || (nSamples < 0 || nSamples > 262144 ) || (gain < 0 || gain > 15 )
    		  || (filter < 0 || filter > 15 ) || (adcClockRate < 1.00 || adcClockRate > 13.8 ) || (triggerSel < 0 || triggerSel > 1 ) || (acqMode < 0 || acqMode > 2 ))
      {
    	  printf("Error: should setup parameters as 'ics710Init(0, 2, 1000, 0, 1, 1.024, 0, 0)': the first card(0), 2(even number) channels, 1000 samples/channel;\n gain is 0--10V input range, filter is 1--10KHz bandwidth, adcClockRate([1.024, 13.8]) is 1.024MHz, triggerSource(0) is internal, acquisition mode(0) is Continuous \n");
    	  return -1;
      }

		ics710Driver *pics710Driver = &ics710Drivers[card];
		pics710Driver->hDevice = -1;

		switch(card)
		{
		case 0:
			{
				pics710Driver->hDevice = open("/dev/ics710-1", O_RDWR);
				break;
			}
		case 1:
			{
				pics710Driver->hDevice = open("/dev/ics710-2", O_RDWR);
				break;
			}
		case 2:
			{
				pics710Driver->hDevice = open("/dev/ics710-3", O_RDWR);
				break;
			}
		case 3:
			{
				pics710Driver->hDevice = open("/dev/ics710-4", O_RDWR);
				break;
			}
		case 4:
			{
				pics710Driver->hDevice = open("/dev/ics710-5", O_RDWR);
				break;
			}
		case 5:
			{
				pics710Driver->hDevice = open("/dev/ics710-6", O_RDWR);
				break;
			}
		case 6:
			{
				pics710Driver->hDevice = open("/dev/ics710-7", O_RDWR);
				break;
			}
		case 7:
			{
				pics710Driver->hDevice = open("/dev/ics710-8", O_RDWR);
				break;
			}
		default:
			break;
		} //switch(card)

		if (0 > pics710Driver->hDevice)
		{
			printf ("Error: cann't open /dev/ics710-%u, '0' for the first card, '1' for the second, etc. check out if ics710.ko is loaded and /dev/ics710[1:8] exists \n",card+1);
			return -1;
		}
		else
		{
			/* blocking I/O: seems efficient enough for 10Hz IOC update */
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
			pics710Driver->control.system_master = ICS710_ENABLE; /* ICS710_ENABLE or ICS710_DISABLE  */
			pics710Driver->control.adc_master = ICS710_ENABLE; /* ICS710_ENABLE or ICS710_DISABLE */
			pics710Driver->control.fpdp_master = ICS710_ENABLE; /* ICS710_ENABLE or ICS710_DISABLE */
			pics710Driver->control.adc_termin = ICS710_ENABLE; /* ICS710_ENABLE or ICS710_DISABLE */
			pics710Driver->control.fpdp_termin = ICS710_ENABLE; /* ICS710_ENABLE or ICS710_DISABLE */
			pics710Driver->control.extclk_term = ICS710_ENABLE; /* ICS710_ENABLE or ICS710_DISABLE*/
			pics710Driver->control.extrig_term = ICS710_ENABLE;/* ICS710_ENABLE or ICS710_DISABLE */
			/* Yong Hu: if using external trigger and Continuous Mode, must set extrig_mode to level(high) control */
			pics710Driver->control.extrig_mode = ICS710_EXTRIG_RISING;/* 0:ICS710_EXTRIG_HIGH,1:ICS710_EXTRIG_RISING,2:ICS710_EXTRIG_LOW, 3: ICS710_EXTRIG_FALLING*/
			pics710Driver->masterControl.board_address = 0;
		/*setup parameters during initialization */
			pics710Driver->control.trigger_select = triggerSel; /*ICS710_TRIG_INTERNAL or ICS710_TRIG_EXTERNAL*/
			pics710Driver->control.acq_mode = acqMode; /* ICS710_CONTINUOUS or ICS710_CAPTURE_NOPRETRG or ICS710_CAPTURE_WITHPRETRG  */
			pics710Driver->gainControl.input_voltage_range = gain;
			//inputRange = 10.0/(1+gainControl.input_voltage_range);
			pics710Driver->filterControl.cutoff_freq_range = filter;
			pics710Driver->totalChannel = totalChannel;
			pics710Driver->nSamples = nSamples;
			pics710Driver->ics710AdcClockRate = adcClockRate; /*5.12MHz*/
 // Yong Hu: if using external trigger and Continuous Mode, must set extrig_mode to level(high) control; disable and re-enable DAQ doesn't work
			if ( (ICS710_TRIG_EXTERNAL == pics710Driver->control.trigger_select) && (ICS710_CONTINUOUS == pics710Driver->control.acq_mode) )
				pics710Driver->control.extrig_mode = ICS710_EXTRIG_HIGH;

			/* Calculated parameters: acquisition length, channel count, buffer length*/
			pics710Driver->acqLength = pics710Driver->nSamples - 1;

			if (pics710Driver->control.packed_data == 0)
				pics710Driver->channelCount = pics710Driver->totalChannel - 1; /* Channel Count Register, for un-packed data */
			else
				pics710Driver->channelCount = pics710Driver->totalChannel / 2 - 1; /* Channel Count Register, for packed data */

			pics710Driver->masterControl.numbers_of_channels = pics710Driver->channelCount;/* For Single Board, the two always equal */

			if (pics710Driver->control.packed_data == 0)
				pics710Driver->bufLength = (pics710Driver->totalChannel * pics710Driver->nSamples / 2) - 1;
			else
				pics710Driver->bufLength = (pics710Driver->totalChannel * pics710Driver->nSamples / 4) - 1;
			if (pics710Driver->bufLength > 262143)
			{
				printf ("Error: Calculated buf_len > 256K(4 MBytes memory board).\n");
				return -1;
			}

			/*allocate DMA memory and clear it*/
			pics710Driver->bytesToRead = (pics710Driver->bufLength + 1) * 8;  /* bytes to be read from ics-710 memory*/
			//pics710Driver->bytesToRead = (pics710Driver->bufLength + 1) * 4;  /* bytes to be read from ics-710 memory*/
			pics710Driver->pAcqData = (long *) ics710AllocateDmaBuffer (pics710Driver->hDevice, pics710Driver->bytesToRead);
			if (NULL == pics710Driver->pAcqData)
			{
				printf ("Error: Not enough memory for %u bytes\n",  pics710Driver->bytesToRead);
				return -1;
			}
			memset (pics710Driver->pAcqData, 0, pics710Driver->bytesToRead);

			/* configure the board */
			if (0 != ics710Config(pics710Driver))
			{
				printf("Error: can't configure the board \n");
				return -1;
			}

			/*debugging*/
			printf("\n********************************************************************************************************************** \n");
			printf("%d channels; %d samples/ch, bufLength: %llu, acqLength: %llu, bytesToRead at each buffer swap: %u, totalDmaBuffer: %d Bytes \n",pics710Driver->totalChannel,
					pics710Driver->nSamples, pics710Driver->bufLength + 1, pics710Driver->acqLength + 1, pics710Driver->bytesToRead, pics710Driver->bytesToRead);

			if(0 == pics710Driver->control.trigger_select)
				printf("internal triggering\n");
			else if (0 == pics710Driver->control.extrig_mode)
				printf("external triggering and trigger mode is high-level\n");
			else if (1 == pics710Driver->control.extrig_mode)
				printf("external triggering and trigger mode is rising-edge \n");

			if (ICS710_CAPTURE_WITHPRETRG == pics710Driver->control.acq_mode)
				printf("Mode of Operation is CaptureWithPreTrigger \n");
			else if (ICS710_CAPTURE_NOPRETRG == pics710Driver->control.acq_mode)
				printf("Mode of Operation is CaptureWithoutPreTrigger \n");
			else
				printf("Mode of Operation is Continuous \n");

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
			//ics710DmaMutex = epicsMutexMustCreate();
			pics710Driver->count = 0;
			snprintf(name, sizeof(name), "tics710Daq%u", card);
			scanIoInit(&pics710Driver->ioscanpvt);
			epicsThreadMustCreate(name,epicsThreadPriorityMin,5000000,ics710DaqThread,pics710Driver);
			printf("spawn a data acquisition thread for ics710 card #%d: %s \n ", card, name);
			printf("********************************************************************************************************************** \n\n");
		}// if (0 > pics710Driver->hDevice) else

	return 0;
  }//  static int ics710Init(int card_totalChannel_nSamples_gain_filter_adcClockRate_triggerSel)

  static const iocshArg ics710InitArg0 = { "card",iocshArgInt};
  static const iocshArg ics710InitArg1 = { "totalChannel",iocshArgInt};
  static const iocshArg ics710InitArg2 = { "nSamples",iocshArgInt};
  static const iocshArg ics710InitArg3 = { "gain",iocshArgInt};
  static const iocshArg ics710InitArg4 = { "filter",iocshArgInt};
  static const iocshArg ics710InitArg5 = { "adcClockRate",iocshArgDouble};
  static const iocshArg ics710InitArg6 = { "triggerSel",iocshArgInt};
  static const iocshArg ics710InitArg7 = { "acqMode",iocshArgInt};

  static const iocshArg * const ics710InitArgs[8] = {
		  &ics710InitArg0, &ics710InitArg1, &ics710InitArg2, &ics710InitArg3,
		  &ics710InitArg4, &ics710InitArg5, &ics710InitArg6, &ics710InitArg7 };

  static const iocshFuncDef ics710InitFuncDef = {"ics710Init",8,ics710InitArgs};

  static void ics710InitCallFunc(const iocshArgBuf *args)
  {
	  ics710Init(args[0].ival, args[1].ival, args[2].ival, args[3].ival,
			     args[4].ival, args[5].dval, args[6].ival, args[7].ival);
  }

  void ics710Registrar()
  {
	  iocshRegister(&ics710InitFuncDef,ics710InitCallFunc);
  }

  epicsExportRegistrar(ics710Registrar);
}
