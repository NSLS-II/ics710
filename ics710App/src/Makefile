TOP=../..
include $(TOP)/configure/CONFIG

#manually copy them from ics710 Linux SDK to 'src', then install them
INC += ics710api.h
LIB_INSTALLS_Linux += ../libics710.a    
#use aSubRecord.h(contains 'not') in .cpp file
CXXFLAGS+=-fno-operator-names 

# Build the IOC application
PROD_IOC = ics710

# ics710.dbd will be created and installed
DBD += ics710.dbd
# ics710.dbd will be made up from these files:
ics710_DBD += base.dbd
#Yong Hu: include dbd for ics710 driver/device support
ics710_DBD += ics710Support.dbd

#include iocStatus and autosave which are installed by epics Debian package
# /usr/lib/epics/dbd/
ics710_DBD += devIocStats.dbd
ics710_DBD += asSupport.dbd
ics710_DBD += system.dbd

# ics710_registerRecordDeviceDriver.cpp derives from ics710.dbd
ics710_SRCS += ics710_registerRecordDeviceDriver.cpp
# works only if the header file(ics710api.h) is installed in $(TOP)/include
# and the static library(libics710.a) is installed in $(TOP)/lib/linux-x86
ics710_SRCS += ics710DrvInit.cpp
ics710_SRCS += ics710DevSupport.cpp
ics710_SRCS += ics710ProcessWfAsub.cpp
# Build the main IOC entry point on workstation OSs.
ics710_SRCS_DEFAULT += ics710Main.cpp

#link to ics710 api libarary (Linux SDK)
ics710_LIBS += ics710

#include autosave and devIocStats libraries which are installed by epics Debian
#package: /usr/lib/epics/lib/linux-x86
ics710_LIBS += autosave
ics710_LIBS += devIocStats

# Finally link to the EPICS Base libraries
ics710_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES

