#!../ics710/bin/linux-x86/ics710

#digitizer ICS-710 at LtB (Linac-to-Booster transferline) for ICTs, etc.
epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","10000000")
epicsEnvSet("ENGINEER","Yong Hu: x3961")
epicsEnvSet("LOCATION","Blg 902 Rm 18") 

dbLoadDatabase "../ics710/dbd/ics710.dbd"
ics710_registerRecordDeviceDriver pdbbase

#ics710Init(int card,int totalChannel,int nSamples,int gain,int filter,double sampleRate,int osr, int triggerSel, int acqMode)
ics710Init(0, 2, 2000, 0, 1, 20.0, 2, 0, 1)
ics710Init(1, 2, 2000, 0, 1, 20.0, 2, 0, 1)

dbLoadRecords ("../ics710/db/ics710Channel.db", "PREFIX=LTB-BI{ICT:1},    CARD=0, CHANNEL=0, NELM=64000")
dbLoadRecords ("../ics710/db/ics710Channel.db", "PREFIX=LTB-BI{ICT:2},    CARD=0, CHANNEL=1, NELM=64000")
dbLoadRecords ("../ics710/db/ics710Card.db",    "PREFIX=LTB-BI{ics710:1}, CARD=0, NELM=64000")

dbLoadRecords ("../ics710/db/ics710Channel.db", "PREFIX=BTS-BI{ICT:1},    CARD=1, CHANNEL=0, NELM=64000")
dbLoadRecords ("../ics710/db/ics710Channel.db", "PREFIX=BTS-BI{ICT:2},    CARD=1, CHANNEL=1, NELM=64000")
dbLoadRecords ("../ics710/db/ics710Card.db",    "PREFIX=BTS-BI{ics710:1}, CARD=1, NELM=64000")

dbLoadRecords ("../ics710/db/iocAdminSoft.db", "IOC=LTB-BI{ICTIOC}")
dbLoadRecords ("../ics710/db/save_restoreStatus.db", "P=LTB-BI{ics710AS}")
save_restoreSet_status_prefix("LTB-BI{ics710AS}")
set_savefile_path("./as", "/save")
set_requestfile_path("./as", "/req")
system("install -d ./as/save")
system("install -d ./as/req")
set_pass0_restoreFile("ics710_settings.sav")

iocInit

makeAutosaveFileFromDbInfo("./as/req/ics710_settings.req", "autosaveFields_pass0")
create_monitor_set("ics710_settings.req", 10, "")
