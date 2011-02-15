#!../../bin/linux-x86/ics710
cd ../..

dbLoadDatabase "dbd/ics710.dbd"
ics710_registerRecordDeviceDriver pdbbase

ics710Init(1)

#dbLoadTemplate "db/userHost.substitutions"
#dbLoadRecords "db/dbSubExample.db", "user=yhuHost"

iocInit

