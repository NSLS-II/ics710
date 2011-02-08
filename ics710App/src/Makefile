TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

LIBRARY_IOC += ics710Support

# xxxRecord.h will be created from xxxRecord.dbd
DBDINC += xxxRecord
# Install devXxxSoft.dbd into <top>/dbd
DBD += xxxSupport.dbd

# Compile and add the code to the support library
ics710Support_SRCS += xxxRecord.c
ics710Support_SRCS += devXxxSoft.c

# Link locally-provided code into the support library,
# rather than directly into the IOC application.
ics710Support_SRCS += dbSubExample.c
ics710Support_SRCS += ics710Hello.c
ics710Support_SRCS += initTrace.c

ics710Support_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# Build the IOC application

PROD_IOC = ics710
# ics710.dbd will be created and installed
DBD += ics710.dbd

# ics710.dbd will be made up from these files:
ics710_DBD += base.dbd
ics710_DBD += xxxSupport.dbd
ics710_DBD += dbSubExample.dbd
ics710_DBD += ics710Hello.dbd
ics710_DBD += initTrace.dbd

# ics710_registerRecordDeviceDriver.cpp derives from ics710.dbd
ics710_SRCS += ics710_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
ics710_SRCS_DEFAULT += ics710Main.cpp
ics710_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#ics710_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from the support library
ics710_LIBS += ics710Support

# NOTE: To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file

ifneq ($(SNCSEQ),)
    # Build sncExample into ics710Support
    sncExample_SNCFLAGS += +r
    ics710_DBD += sncExample.dbd
    ics710Support_SRCS += sncExample.stt
    ics710Support_LIBS += seq pv
    ics710_LIBS += seq pv

    # Build sncProgram as a standalone program
    PROD_HOST += sncProgram
    sncProgram_SNCFLAGS += +m
    sncProgram_SRCS += sncProgram.st
    sncProgram_LIBS += seq pv
    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
endif

# Finally link to the EPICS Base libraries
ics710_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

