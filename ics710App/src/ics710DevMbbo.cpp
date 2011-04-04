/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */
/* ics710DevMbbo.cpp: device support for mbbo record; reconfigure parameters: gain, cut-off frequency, osr, triggerSel, acquisition mode, start/stop  */

#include "ics710Dev.h"

#include <devSup.h>
#include <epicsExport.h>
#include <mbboRecord.h>

extern "C" {
  static long init_record(void* record)
  {
	    mbboRecord* r = reinterpret_cast<mbboRecord*>(record);
	    int status = ics710InitRecord(r, r->out);
	    return status;
  }

  static long write_mbbo(void* record)
  {
    mbboRecord* r = reinterpret_cast<mbboRecord*>(record);
    return ics710WriteRecord(r);
  }

  struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_mbbo;
  } ics710DevMbbo = {
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    write_mbbo
  };
  epicsExportAddress(dset, ics710DevMbbo);
}
