/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */
/*ics710DevInit.cpp:  3 basic operations for each device support: initialize, read, write*/

#include "ics710Dev.h"

#include <dbAccess.h>
#include <recGbl.h>
#include <devSup.h>
#include <epicsExport.h>
#include <alarm.h>
#include <link.h>

#include <stdio.h>
#include <stdlib.h>

static int ics710DadField(void* record, const char* message, const char* fieldname)
{
	  fprintf(stderr, "ics710InitRecord: %s %s\n", message, fieldname);
	  recGblRecordError(S_db_badField, record, message);
	  return S_db_badField;
}

/*defined in ics710Dev.h*/
template<class T> int ics710InitRecord(T* record, DBLINK link)
{
	  int status;

	  if (link.type != INST_IO)
	    return ics710DadField(record, "wrong link type, must be INST_IO @", "");

	  struct instio* pinstio = &link.value.instio;
	  if (!pinstio->string)
		  return ics710DadField(record, "invalid link, must not be empty", "");

	  ics710RecPrivate* pics710RecPrivate = new ics710RecPrivate;
	  //ics710RecPrivate *pics710RecPrivate = (ics710RecPrivate *)malloc(sizeof(ics710RecPrivate));//works, but need 'stdlib.h'

	  /*parse the link string: must have 'card' and 'name'*/
	  const char* sinp = pinstio->string;
	  status = sscanf(sinp, "C%u S%u %s", &pics710RecPrivate->card, &pics710RecPrivate->channel, pics710RecPrivate->name);
	  if (status != 3)
	  {
		    status = sscanf(sinp, "C%u %s", &pics710RecPrivate->card, pics710RecPrivate->name);
		    if (status != 2)
		    {
			      delete pics710RecPrivate;
			      return ics710DadField(record, "cannot parse INP field", sinp);
		    }
	  }

	  /* save the parsed results: #card(0), #channel(0), #name(WRAW)*/
	  record->dpvt = pics710RecPrivate;

	  /* initialize each record specifically*/
	  status = ics710InitRecordSpecialized(record);
	  if (status)
	  {
		    record->dpvt = 0;
		    delete pics710RecPrivate;
		    return ics710DadField(record, "cannot find record name", sinp);
	  }

	  return 0;
}

/*defined in ics710Dev.h*/
template<class T> int ics710ReadRecord(T* record)
{
  int status = ics710ReadRecordSpecialized(record);
  if (status)
  {
	    record->nsta = UDF_ALARM;
	    record->nsev = INVALID_ALARM;
	    return -1;
  }
  return 0;
}

/*defined in ics710Dev.h*/
template<class T> int ics710WriteRecord(T* record)
{
  int status = ics710WriteRecordSpecialized(record);
  if (status)
  {
	    record->nsta = UDF_ALARM;
	    record->nsev = INVALID_ALARM;
	    return -1;
  }
  return 0;
}

#include <longoutRecord.h> /*configure 3 parameters: totalChannel, nSamples, samplingRate*/
#include <mbboRecord.h> /*configure 5 parameters: gain, filter,osr,triggerSel, acqMode*/
#include <waveformRecord.h> /*put the acquired data of individual channel to waveform record*/

template int ics710InitRecord(longoutRecord*,  DBLINK);
template int ics710InitRecord(mbboRecord*,     DBLINK);
template int ics710InitRecord(waveformRecord*, DBLINK);
template int ics710WriteRecord(longoutRecord*);
template int ics710WriteRecord(mbboRecord*);
template int ics710ReadRecord(waveformRecord*);

