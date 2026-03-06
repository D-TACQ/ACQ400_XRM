TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Use typed rset structure (see 3.16.1 release notes)
USR_CPPFLAGS += -DUSE_TYPED_RSET

# xxxRecord.h will be created from xxxRecord.dbd
DBDINC += xxxRecord

# Install xxxSupport.dbd into <top>/dbd
DBD += xxxSupport.dbd

# Build an IOC support library
LIBRARY_IOC += xrmIocSupport

# Compile and add code to the support library
xrmIocSupport_SRCS += xxxRecord.c
xrmIocSupport_SRCS += devXxxSoft.c

# Link locally-provided code into the support library,
# rather than directly into the IOC application, that
# causes problems on Windows DLL builds
xrmIocSupport_SRCS += dbSubExample.c
xrmIocSupport_SRCS += devxrmIocVersion.c
xrmIocSupport_SRCS += xrmIocHello.c
xrmIocSupport_SRCS += initTrace.c


xrmIocSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

# Auto-generate a header file containing a version string.
# Version comes from the VCS if available, else date+time.
GENVERSION = xrmIocVersion.h
# Macro name
GENVERSIONMACRO = xrmIocVERSION

# Build the IOC application
PROD_IOC = xrmIoc

# xrmIoc.dbd will be created and installed
DBD += xrmIoc.dbd

# xrmIoc.dbd will include these files:
xrmIoc_DBD += base.dbd
xrmIoc_DBD += xxxSupport.dbd
xrmIoc_DBD += dbSubExample.dbd
xrmIoc_DBD += devxrmIocVersion.dbd
xrmIoc_DBD += xrmIocHello.dbd
xrmIoc_DBD += initTrace.dbd
xrmIoc_DBD += acq400_FMT_sim.dbd
xrmIoc_DBD += acq400_FMT_rx.dbd
xrmIoc_DBD += acq400_SOE.dbd
xrmIoc_DBD += acq400_PM.dbd

# xrmIoc_registerRecordDeviceDriver.cpp derives from xrmIoc.dbd
xrmIoc_SRCS += xrmIoc_registerRecordDeviceDriver.cpp
xrmIoc_SRCS += xrm_struct_support.cpp
xrmIoc_SRCS += xrm_struct_test.cpp
xrmIoc_SRCS += acq400_asynPortDriver.cpp
xrmIoc_SRCS += acq400_FMT_abc.cpp
xrmIoc_SRCS += acq400_FMT_sim.cpp
xrmIoc_SRCS += acq400_FMT_rx.cpp
xrmIoc_SRCS += acq400_SOE.cpp
xrmIoc_SRCS += acq400_PM.cpp
# this should be a lib from ACQ400DRV, copy bodily for now..
xrmIoc_SRCS += Multicast.cpp

# Build the main IOC entry point where needed
xrmIoc_SRCS_DEFAULT += xrmIocMain.cpp
xrmIoc_SRCS_vxWorks += -nil-

# Link in the code from our support library
xrmIoc_LIBS += xrmIocSupport
xrmIoc_LIBS += acq

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
ifneq ($(SNCSEQ),)
    $(info pgmwashere SNCSEQ $(SNCSEQ))
    # Build sncExample into xrmIocSupport
    sncExample_SNCFLAGS += +r
    xrmIoc_DBD += sncExample.dbd
    # A .stt sequence program is *not* pre-processed:
    xrmIocSupport_SRCS += sncExample.stt
    xrmIocSupport_LIBS += seq pv
    xrmIoc_LIBS += seq pv

    # Build sncProgram as a standalone program
    PROD_HOST += sncProgram
    sncProgram_SNCFLAGS += +m
    # A .st sequence program *is* pre-processed:
    sncProgram_SRCS += sncProgram.st
    sncProgram_LIBS += seq pv
    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
endif

# XRM D-TACQ mandated
xrmIoc_DBD += asyn.dbd
xrmIoc_DBD += drvAsynIPPort.dbd
xrmIoc_DBD += stream.dbd



# XRM customer mandated features

xrmIoc_DBD += reccaster.dbd
xrmIoc_DBD += softIocPVX.dbd
xrmIoc_DBD += pvxsIoc.dbd

xrmIoc_LIBS += pvxs
xrmIoc_LIBS += pvxsIoc
xrmIoc_LIBS += reccaster

# Finally link IOC to the EPICS Base libraries
xrmIoc_LIBS += $(EPICS_BASE_IOC_LIBS)

PROD_SYS_LIBS_DEFAULT += stream asyn pcre fftw3f

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

# Explicit dependency needed for generated header file
devxrmIocVersion$(DEP): $(COMMON_DIR)/$(GENVERSION)
