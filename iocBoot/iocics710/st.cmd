#!../../bin/linux-x86/ics710

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","10000000")
epicsEnvSet("ENGINEER","Yong Hu: x3961")
epicsEnvSet("LOCATION","Blg 902 Rm 18")

cd ../..

dbLoadDatabase "dbd/ics710.dbd"
ics710_registerRecordDeviceDriver pdbbase

#static int ics710Init(int card,int totalChannel,int nSamples,int gain,int filter,double adcClockRate,int osr, int triggerSel, int acqMode)
#int osr = 2; /* oversampling ratio: 0:ICS710_SAMP_NORMAL,1:ICS710_SAMP_DOUBLE,2:ICS710_SAMP_QUAD*/
#int triggerSel = 1; /*0: internal; 1: external triggering*/
#int acqMode = 1; /*0: Continuous, 1: ICS710_CAPTURE_NOPRETRG, 2: ICS710_CAPTURE_WITHPRETRG*/
#For CBLM at NSLS, 1.2Hz external trigger, 200.0KS/s(12.8MHz clock), 500ms length
ics710Init(0, 8, 10000, 0, 10, 200.0, 2, 1, 1)

dbLoadTemplate "db/ics710Channel.substitutions"
dbLoadTemplate "db/ics710Card.substitutions"

iocInit

