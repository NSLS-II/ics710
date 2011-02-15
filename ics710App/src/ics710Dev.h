/*Yong Hu: 02-08-2010*/

#ifndef ICS710_DEV_H
#define ICS710_DEV_H

#include <dbScan.h>
#include <link.h>

struct ics710_record_t
{
  int module;
  int channel;
  char name[8];
  void* pvt;
};

typedef struct ics710_record_t rec_t;

template<class T> int ics710_init_record(T* record, DBLINK link);
template<class T> int ics710_read_record(T* record);
template<class T> int ics710_write_record(T* record);
template<class T> int ics710_init_record_specialized(T* record);
template<class T> int ics710_read_record_specialized(T* record);
template<class T> int ics710_write_record_specialized(T* record);
template<class T> IOSCANPVT ics710_getioscanpvt_specialized(T* record);

#endif
