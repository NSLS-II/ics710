#!../../bin/linux-x86/ics710

#This startup script is intended for NSLS-2 application
#you should change it for your specific application 
#digitizer ICS-710 at LtB (Linac-to-Booster transferline) for Bergoz ICTs, etc.
epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","10000000")
epicsEnvSet("ENGINEER","Yong Hu: x3961")
epicsEnvSet("LOCATION","Blg 902 Rm 18") 

cd ../..

dbLoadDatabase "dbd/ics710.dbd"
ics710_registerRecordDeviceDriver pdbbase

#ics710Init(int card,int totalChannel)
#card: [0, 7]; totalChannel: [2, 32] && (totalChannel%2 == 0)
#the first card is '0'; totalChannel should be an even number
ics710Init(0, 2)

#MON: beam MONitor/detector;
#CARD: the first card/digitizer is 0;
#CHANNEL: the first channel of the card is 0;
#NELM: max. number of samples/channel;
#DIG:ICT:the DIGitizer for 2 ICTs;
dbLoadRecords("db/ics710Channel.db","MON=LTB-BI{ICT:1},CARD=0,CHANNEL=0,NELM=64000")
dbLoadRecords("db/ics710Channel.db","MON=LTB-BI{ICT:2},CARD=0,CHANNEL=1,NELM=64000")
dbLoadRecords("db/ics710Card.db","DIG=LTB-BI{DIG:ICT}, CARD=0, NELM=64000")

#devIocStats and autosave stuff
#dbLoadRecords ("../ics710/db/iocAdminSoft.db", "IOC=LTB-BI{ICTIOC}")
#dbLoadRecords ("../ics710/db/save_restoreStatus.db", "P=LTB-BI{ics710AS}")
#save_restoreSet_status_prefix("LTB-BI{ics710AS}")
#set_savefile_path("./as", "/save")
#set_requestfile_path("./as", "/req")
#system("install -d ./as/save")
#system("install -d ./as/req")
#set_pass0_restoreFile("ics710_settings.sav")

iocInit

#makeAutosaveFileFromDbInfo("./as/req/ics710_settings.req", "autosaveFields_pass0")
#create_monitor_set("ics710_settings.req", 10, "")
