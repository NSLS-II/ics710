#!../../bin/linux-x86/ics710

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","10000000")
epicsEnvSet("ENGINEER","Yong Hu: x3961")
epicsEnvSet("LOCATION","Blg 902 Rm 18")
epicsEnvSet "EPICS_CAS_AUTO_BEACON_ADDR_LIST", "NO"
epicsEnvSet "EPICS_CAS_BEACON_ADDR_LIST", "130.199.195.255:5065"
epicsEnvSet "EPICS_CAS_SERVER_PORT", "8002" 

cd ../..

dbLoadDatabase "dbd/ics710.dbd"
ics710_registerRecordDeviceDriver pdbbase

#ics710Init(int card,int totalChannel,int nSamples,int gain,int filter,double sampleRate,int osr, int triggerSel, int acqMode)
#fix totalChannel =8: NSLS-2 ICS-710 has 8 channels; each channel/ADC should be calibrated after power-up; max. 64K samples/ch
#sampleRate:set OSR = 0 for speed range [1KHz,54KHz] to get better SNR; set OSR=2 to get the highest speed ~200KHz  
#int osr = 2; /* oversampling ratio: 0:ICS710_SAMP_NORMAL,1:ICS710_SAMP_DOUBLE,2:ICS710_SAMP_QUAD*/
#int triggerSel = 1; /*0: internal; 1: external triggering*/
#int acqMode = 1; /*0: Continuous, 1: ICS710_CAPTURE_NOPRETRG, 2: ICS710_CAPTURE_WITHPRETRG*/

#For CBLMs at NSLS, 1.2Hz trigger, 200.0KS/s(12.8MHz clock), 160ms acquisition length(32K Samples)
#ics710Init(0, 8, 32000, 0, 10, 200.0, 2, 1, 1)

#Test DCCT test: 8-ch/card, 1K samples/ch, 10V range, 10KHz bw, 1KS/s, over-sampling ratio is 128 , internal trigger, continous acquisition
# must set OSR = 0 to get the lowest speed 1KS/s
ics710Init(0, 8, 1000, 0, 1, 1.0, 0, 0, 0)

# for Linac ICTs, 300 us * 200KS/s = 60 samples, pluse fake/garbage data 1024/4=256, 60+256 ~ 320
#ics710Init(0, 8, 320, 0, 1, 200.0, 2, 1, 1)

dbLoadTemplate "db/ics710Channel.substitutions"
dbLoadTemplate "db/ics710Card.substitutions"

iocInit

