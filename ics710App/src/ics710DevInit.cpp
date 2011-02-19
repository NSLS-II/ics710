/*Yong Hu: 02-08-2010*/
#include "ics710Dev.h"

#include <dbAccess.h>
#include <recGbl.h>
#include <devSup.h>
#include <epicsExport.h>
#include <alarm.h>
#include <link.h>

#include <stdio.h>

static int ics710DadField(void* record, const char* message, const char* fieldname)
{
	  fprintf(stderr, "ics710InitRecord: %s %s\n", message, fieldname);
	  recGblRecordError(S_db_badField, record, message);
	  return S_db_badField;
}

template<class T> int ics710InitRecord(T* record, DBLINK link)
{
	  int status;

	  if (link.type != INST_IO)
	    return ics710DadField(record, "wrong link type", "");

	  struct instio* pinstio = &link.value.instio;
	  if (!pinstio->string)
		  return ics710DadField(record, "invalid link", "");

	  ics710RecPrivate* pics710RecPrivate = new ics710RecPrivate;
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

	  record->dpvt = pics710RecPrivate;

	  status = ics710InitRecordSpecialized(record);
	  if (status)
	  {
		    record->dpvt = 0;
		    delete pics710RecPrivate;
		    return ics710DadField(record, "cannot find record name", sinp);
	  }

	  return 0;
}

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
/*
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
*/

//#include <longinRecord.h>
//#include <longoutRecord.h>
//#include <aiRecord.h>
//#include <aoRecord.h>
//#include <mbboRecord.h>
#include <waveformRecord.h>

//template int acqiris_init_record(longinRecord*,   DBLINK);
//template int acqiris_init_record(longoutRecord*,  DBLINK);
//template int acqiris_init_record(aiRecord*,       DBLINK);
//template int acqiris_init_record(aoRecord*,       DBLINK);
//template int acqiris_init_record(mbboRecord*,     DBLINK);
template int ics710InitRecord(waveformRecord*, DBLINK);
//template int acqiris_read_record(longinRecord*);
//template int acqiris_read_record(longoutRecord*);
//template int acqiris_read_record(aiRecord*);
//template int acqiris_read_record(aoRecord*);
//template int acqiris_read_record(mbboRecord*);
template int ics710ReadRecord(waveformRecord*);
//template int acqiris_write_record(longoutRecord*);
//template int acqiris_write_record(aoRecord*);
//template int acqiris_write_record(mbboRecord*);

