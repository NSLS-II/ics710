#!../../bin/linux-x86/ics710

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","10000000")
epicsEnvSet("ENGINEER","Yong Hu: x3961")
epicsEnvSet("LOCATION","Blg 902 Rm 18")

cd ../..

dbLoadDatabase "dbd/ics710.dbd"
ics710_registerRecordDeviceDriver pdbbase

#static int ics710Init(int card,int totalChannel,int nSamples,int gain,int filter,double sampleRate,int osr, int triggerSel, int acqMode)
#int osr = 2; /* oversampling ratio: 0:ICS710_SAMP_NORMAL,1:ICS710_SAMP_DOUBLE,2:ICS710_SAMP_QUAD*/
#int triggerSel = 1; /*0: internal; 1: external triggering*/
#int acqMode = 1; /*0: Continuous, 1: ICS710_CAPTURE_NOPRETRG, 2: ICS710_CAPTURE_WITHPRETRG*/

#For CBLMs at NSLS, 1.2Hz trigger, 200.0KS/s(12.8MHz clock), 160ms acquisition length(32K Samples)
ics710Init(0, 2, 32000, 0, 10, 200.0, 2, 1, 1)

# for Linac ICTs, 300 us * 200KS/s = 60 samples, pluse fake/garbage data 1024/4=256, 60+256 ~ 320
#ics710Init(0, 4, 320, 0, 1, 200.0, 2, 1, 1)

#For calibration: 2-ch, 2K Samples/ch, 10V range, 10KHz bw, 4.0KS/s,osr=0, external trigger, captureNoPre
#ics710Init(0, 2, 2000, 0, 1, 4.0, 0, 1, 1)

dbLoadTemplate "db/ics710Channel.substitutions"
dbLoadTemplate "db/ics710Card.substitutions"

iocInit

