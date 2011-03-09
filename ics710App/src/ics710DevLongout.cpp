/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */

#include "ics710Dev.h"

#include <devSup.h>
#include <epicsExport.h>
#include <longoutRecord.h>

extern "C" {
  static long init_record(void* record)
  {
	    longoutRecord* r = reinterpret_cast<longoutRecord*>(record);
	    int status = ics710InitRecord(r, r->out);

	    return status;
  }

  static long write_longout(void* record)
  {
    longoutRecord* r = reinterpret_cast<longoutRecord*>(record);
    return ics710WriteRecord(r);
  }

  struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_longout;
  } ics710DevLongout = {
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    write_longout
  };
  epicsExportAddress(dset, ics710DevLongout);
}
