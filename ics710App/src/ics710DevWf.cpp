/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */
/*ics710DevWf.cpp: device support for waveform record, put the acquired data of individual channel to waveform record
 * functions called sequence: ics710DevWf(Longout,Mbbo).cpp --> ics710DevInit.cpp --> ics710DrvWf(Longout,Mbbo).cpp
 * */

#include "ics710Dev.h"

#include <devSup.h>
#include <dbScan.h>
#include <epicsExport.h>
#include <waveformRecord.h>

extern "C"
{
/* init_record() at ics710DevWf.cpp --> ics710InitRecord() in ics710DevInit.cpp --> ics710InitRecordSpecialized() in ics710DrvWf.cpp */
  static long init_record(void* record)
  {
	    waveformRecord* r = reinterpret_cast<waveformRecord*>(record);
	    return ics710InitRecord(r, r->inp);
  }

  /* read_wf() at ics710DevWf.cpp --> ics710ReadRecord() in ics710DevInit.cpp --> ics710ReadRecordSpecialized() in ics710DrvWf.cpp */
  static long read_wf(void* record)
  {
	    waveformRecord* r = reinterpret_cast<waveformRecord*>(record);
	    return ics710ReadRecord(r);
  }

  /* get_ioint_info() at ics710DevWf.cpp -->  ics710GetioscanpvtSpecialized() in ics710DrvWf.cpp */
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
