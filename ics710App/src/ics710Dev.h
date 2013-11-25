/* Yong Hu: started on 02-08-2011
 * Prototype IOC fully functions on 03-03-2011
 * */

#ifndef ICS710_DEV_H
#define ICS710_DEV_H

#include "devSup.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* record private: get No.card and No.channel from 'Instrument Address'
 *  @C${CARD} S${CHANNEL}
 *  */
typedef struct
{
    int card;
    int channel;
//char name[8];
} ics710RecPrivate;

//common device support entry table
typedef struct dsetCommon
{
    long number;
    DEVSUPFUN devReport;
    DEVSUPFUN init;
    DEVSUPFUN initRecord;
    DEVSUPFUN getIointInfo;
    DEVSUPFUN writeOrRead;
    DEVSUPFUN specialConv;
} dsetCommon;

#ifdef __cplusplus
}
#endif

#endif//#ifndef ICS710_DEV_H
