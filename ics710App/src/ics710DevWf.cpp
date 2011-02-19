/*Yong Hu: 02-08-2010*/
#include "ics710Dev.h"

#include <devSup.h>
#include <dbScan.h>
#include <epicsExport.h>
#include <waveformRecord.h>

extern "C"
{
  static long init_record(void* record)
  {
	    waveformRecord* r = reinterpret_cast<waveformRecord*>(record);
	    return ics710InitRecord(r, r->inp);
  }

  static long read_wf(void* record)
  {
	    waveformRecord* r = reinterpret_cast<waveformRecord*>(record);
	    return ics710ReadRecord(r);
  }

  static long get_ioint_info(int cmd, void* record, IOSCANPVT* ppvt)
  {  
	    waveformRecord* r = reinterpret_cast<waveformRecord*>(record);
	    *ppvt = ics710GetioscanpvtSpecialized(r);
	    return 0;
  }

  struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_wf;
  } ics710DevWf = {
    5,
    NULL,
    NULL,
    init_record,
    (DEVSUPFUN)get_ioint_info,
    read_wf
  };
  epicsExportAddress(dset, ics710DevWf);
}
