/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */

#ifndef ICS710_DEV_H
#define ICS710_DEV_H

#include <dbScan.h>
#include <link.h>

/* record private: get No.card, No.channel and string from 'Instrument Address' @C${CARD} S${CHANNEL} WRAW*/
struct ics710RecPrivate
{
  int card;
  int channel;
  char name[8];
  void* pvt; /*for function selection*/
};

/*these templates are implemented in ics710DevInit.cpp and called by device support routines(ics710DevLongout/Mbbo/Wf.cpp)*/
template<class T> int ics710InitRecord(T* record, DBLINK link);
template<class T> int ics710ReadRecord(T* record);
template<class T> int ics710WriteRecord(T* record);

/*this template is implemented for each record (ics710DrvLongout/Mbbo/Wf.cpp) and called by ics710InitRecord()*/
template<class T> int ics710InitRecordSpecialized(T* record);

/*this template is implemented for input record: ics710DrvWf.cpp and called by ics710ReadRecord()*/
template<class T> int ics710ReadRecordSpecialized(T* record);

/*this template is implemented for output record: ics710DrvLongout/Mbbo.cpp called by ics710WriteRecord()*/
template<class T> int ics710WriteRecordSpecialized(T* record);

/*this template is implemented for waveform record I/O intr: ics710DrvWf.cpp*/
template<class T> IOSCANPVT ics710GetioscanpvtSpecialized(T* record);

#endif
