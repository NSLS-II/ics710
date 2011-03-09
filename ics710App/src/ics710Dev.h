/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */

#ifndef ICS710_DEV_H
#define ICS710_DEV_H

#include <dbScan.h>
#include <link.h>

struct ics710RecPrivate
{
  int card;
  int channel;
  char name[8];
  void* pvt;
};

//typedef struct ics710RecPrivate rec_t;

template<class T> int ics710InitRecord(T* record, DBLINK link);
template<class T> int ics710ReadRecord(T* record);
template<class T> int ics710WriteRecord(T* record);
template<class T> int ics710InitRecordSpecialized(T* record);
template<class T> int ics710ReadRecordSpecialized(T* record);
template<class T> int ics710WriteRecordSpecialized(T* record);
template<class T> IOSCANPVT ics710GetioscanpvtSpecialized(T* record);

#endif
