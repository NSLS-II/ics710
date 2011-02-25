#!../../bin/linux-x86/ics710

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","10000000")
epicsEnvSet("ENGINEER","Yong Hu: x3961")
epicsEnvSet("LOCATION","Blg 902 Rm 18")

cd ../..

dbLoadDatabase "dbd/ics710.dbd"
ics710_registerRecordDeviceDriver pdbbase

#static int ics710Init(int card,int totalChannel,int nSamples,int gain,int filter,int adcClockRate,int triggerSel, int acqMode)
#unsigned triggerSel = 0; /*0: internal; 1: external triggering*/
#unsigned acqMode = 2; /*0: Continuous, 1: ICS710_CAPTURE_NOPRETRG, 2: ICS710_CAPTURE_WITHPRETRG*/
ics710Init(0, 2, 2000, 0, 1, 1.024, 1, 0)
#ics710Init(0, 2, 1000, 0, 1, 5.120, 1, 0)

#dbLoadTemplate "db/userHost.substitutions"
#dbLoadRecords "db/dbSubExample.db", "user=yhuHost"
dbLoadTemplate "db/ics710Channel.substitutions"

iocInit

