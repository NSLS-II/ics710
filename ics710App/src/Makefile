TOP=../..
include $(TOP)/configure/CONFIG

#Yong Hu: check out areaDetector/prosilica to use external header files and static library 
#manually copy them from ics710 Linux SDK to 'src', then install them
INC += ics710api.h
LIB_INSTALLS_Linux += ../libics710.a    

# Build the IOC application
PROD_IOC = ics710

# ics710.dbd will be created and installed
DBD += ics710.dbd
# ics710.dbd will be made up from these files:
ics710_DBD += base.dbd
#Yong Hu: include dbd for ics710 driver/device support
ics710_DBD += ics710Support.dbd

# ics710_registerRecordDeviceDriver.cpp derives from ics710.dbd
ics710_SRCS += ics710_registerRecordDeviceDriver.cpp
#Yong Hu: works only if the header file(ics710api.h) is installed in $(TOP)/include
# and the static library(libics710.a) is installed in $(TOP)/lib/linux-x86
ics710_SRCS += ics710DrvInit.cpp
ics710_SRCS += ics710Daq.cpp 
ics710_SRCS += ics710DevInit.cpp
ics710_SRCS += ics710DrvWf.cpp
ics710_SRCS += ics710DevWf.cpp 
ics710_SRCS += ics710DrvMbbo.cpp
ics710_SRCS += ics710DevMbbo.cpp 
ics710_SRCS += ics710DrvLongout.cpp
ics710_SRCS += ics710DevLongout.cpp
ics710_SRCS += ics710Asub.c
# Build the main IOC entry point on workstation OSs.
ics710_SRCS_DEFAULT += ics710Main.cpp

#Yong Hu: link to ics710 api libarary (Linux SDK)
ics710_LIBS += ics710
#Yong Hu: it doesn't work for static library, will get error like undefined reference to `ics710FreeDmaBuffer'
#USR_CFLAGS += -DREDHAT61 -I../../../ics710/inc
#USR_CPPFLAGS += -DREDHAT61 -I../../../ics710/inc
#ics710_LDFLAGS += -L../../../ics710/api -lics710

# Finally link to the EPICS Base libraries
ics710_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES

