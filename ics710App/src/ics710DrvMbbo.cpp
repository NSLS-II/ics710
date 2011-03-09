/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */

#include "ics710api.h"

#include "ics710Dev.h"
#include "ics710Drv.h"

#include <epicsMutex.h>
#include <mbboRecord.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_MBBO_FUNC 6

static int setGain(ics710Driver *pics710Driver, int val)
{
	int errorCode;

	if (val == pics710Driver->gainControl.input_voltage_range) return 0;

	pics710Driver->gainControl.input_voltage_range = val;
	if (ICS710_OK != (errorCode = ics710GainSet (pics710Driver->hDevice, &(pics710Driver->gainControl))))
	{
		printf("can't set gain control register, errorCode: %d \n", errorCode);
		//releaseBoard(pics710Driver);
		return errorCode;
	}

	printf("reconfigure gain/input voltage range to: %.3fV \n", 10.00 / (1+pics710Driver->gainControl.input_voltage_range));
	return 0;
}

static int setFilter(ics710Driver *pics710Driver, int val)
{
	int errorCode;

	if (pics710Driver->filterControl.cutoff_freq_range == val) return 0;

	pics710Driver->filterControl.cutoff_freq_range = val;
	if (ICS710_OK != (errorCode = ics710FilterSet (pics710Driver->hDevice, &(pics710Driver->filterControl))))
	{
		printf("can't set filter control register, errorCode: %d \n", errorCode);
		//releaseBoard(pics710Driver);
		return errorCode;
	}

	printf("reconfigure filter/cut-off frequency to: %dKHz\n", 10*pics710Driver->filterControl.cutoff_freq_range);
	return 0;
}

static int setOsr(ics710Driver *pics710Driver, int val)
{
	int errorCode;

	if (pics710Driver->control.oversamp_ratio == val) return 0;

	pics710Driver->control.oversamp_ratio = val;//*0:ICS710_SAMP_NORMAL,1:ICS710_SAMP_DOUBLE,2:ICS710_SAMP_QUAD
	if (ICS710_OK != (errorCode = ics710ControlSet (pics710Driver->hDevice, &(pics710Driver->control))))
	{
		printf("can't set control register(over-sampling ratio), errorCode: %d \n", errorCode);
		return errorCode;
	}
	printf("reconfigure data output rate by setOsr: %3.3f KHz \n", 1000 * pics710Driver->actualADCRate / (256/(1<<val)));

	if (ICS710_OK != (errorCode = ics710Enable (pics710Driver->hDevice)))
	{
		printf("can't enable the board in ics710DrvMbbo.cpp/setOsr, errorCode: %d \n", errorCode);
		return errorCode;
	}

	return 0;
}

static int setTrigger(ics710Driver *pics710Driver, int val)
{
	int errorCode;

	if (pics710Driver->control.trigger_select == val) return 0;

	pics710Driver->control.trigger_select = val; //ICS710_TRIG_INTERNAL/0 or ICS710_TRIG_EXTERNAL/1
	if (ICS710_OK != (errorCode = ics710ControlSet (pics710Driver->hDevice, &(pics710Driver->control))))
	{
		printf("can't set control register(triggering), errorCode: %d \n", errorCode);
		return errorCode;
	}
	printf("reconfigure triggering to: %s \n", (0 == val)?"internal":"external");

	if (ICS710_OK != (errorCode = ics710Enable (pics710Driver->hDevice)))
	{
		printf("can't enable the board in ics710DrvMbbo.cpp/setTrigger, errorCode: %d \n", errorCode);
		return errorCode;
	}

	return 0;
}

static int setAcqMode(ics710Driver *pics710Driver, int val)
{
	int errorCode;

	if (val == pics710Driver->control.acq_mode) return 0;

	pics710Driver->control.acq_mode = val; // 0:ICS710_CONTINUOUS or 1:ICS710_CAPTURE_NOPRETRG or 2:ICS710_CAPTURE_WITHPRETRG
	if (ICS710_OK != (errorCode = ics710ControlSet (pics710Driver->hDevice, &(pics710Driver->control))))
	{
		printf("can't set control register(Acquisition Mode), errorCode: %d \n", errorCode);
		return errorCode;
	}
	printf("reconfigure Acquisition Mode to: %s \n", (0==pics710Driver->control.acq_mode) ? "Continuous":"Capture");

	if (ICS710_OK != (errorCode = ics710Enable (pics710Driver->hDevice)))
	{
		printf("can't enable the board in ics710DrvMbbo.cpp/setAcqMode, errorCode: %d \n", errorCode);
		return errorCode;
	}

	return 0;
}

static int setRunning(ics710Driver *pics710Driver, int val)
{
	  bool start_run = !pics710Driver->running && val;
	  pics710Driver->running = val;
	  if (start_run)
	  {
		    printf("start/restart DAQ \n");
		    pics710Driver->count = 0;
		    pics710Driver->timeouts = 0;
		    pics710Driver->readErrors = 0;
		    pics710Driver->truncated = 0;
		    epicsEventSignal(pics710Driver->runSemaphore);
	  }

	  return 0;
}

typedef int (*ics710MbboFunc)(ics710Driver *pics710Driver, int val);
struct ics710MbboFuncStruct
{
	  ics710MbboFunc wfunc;
};
static struct
{
  const char* name;
  ics710MbboFunc wfunc;
} parseMbboString[MAX_MBBO_FUNC] =
{
  {"MGAI", setGain},
  {"MFIL", setFilter},
  {"MOSR", setOsr},
  {"MTRI", setTrigger},
  {"MACQ", setAcqMode},
  {"MRUN", setRunning},
};

template<> int ics710InitRecordSpecialized(mbboRecord* pmbbo)
{
	  int i;
	  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(pmbbo->dpvt);
	  ics710Debug("ics710IniMbboRecord: record name: %s, card: %d,link name: %s \n", pmbbo->name, pics710RecPrivate->card, pics710RecPrivate->name);

	  for (i = 0; i < MAX_MBBO_FUNC; i++)
	  {
	       if (0 == strcmp(pics710RecPrivate->name, parseMbboString[i].name))
	       {
		    	ics710MbboFuncStruct* pics710MbboFuncStruct = new ics710MbboFuncStruct;
		    	pics710MbboFuncStruct->wfunc = parseMbboString[i].wfunc;
		        pics710RecPrivate->pvt = pics710MbboFuncStruct;
		        ics710Debug("parseMbboString[i].name: %s \n", parseMbboString[i].name);
		        return 0;
	       }
	  }

	  return -1;
}

template<> int ics710WriteRecordSpecialized(mbboRecord* pmbbo)
{
	  int val, status;
	  ics710RecPrivate* pics710RecPrivate = reinterpret_cast<ics710RecPrivate*>(pmbbo->dpvt);
	  ics710Driver* pics710Driver = &ics710Drivers[pics710RecPrivate->card];
	  ics710MbboFuncStruct* pics710MbboFuncStruct = reinterpret_cast<ics710MbboFuncStruct*>(pics710RecPrivate->pvt);

	  val = pmbbo->val;
	  status = pics710MbboFuncStruct->wfunc(pics710Driver, val);

	  return status;
}
