/*
 * acq400_asynPortDriver.h
 *
 *  Created on: 6 Mar 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_ASYNPORTDRIVER_H_
#define XRMIOCAPP_SRC_ACQ400_ASYNPORTDRIVER_H_

#include "asynPortDriver.h"

#define PS_RUNSTOP	"RUNSTOP"	/* asynInt32, r/w */
#define PS_UPDATES	"UPDATES"	/* asynInt32, r/c */
#define PS_TS_USEC	"TS_USEC"       /* asynInt32, ro  */

class acq400_asynPortDriver: public asynPortDriver {
protected:
	asynStatus gip(int pnum, int* pram);
	asynStatus gip(int addr, int pnum, int* pram);
	asynStatus sip(int addr, int pnum, int pram);
	asynStatus sip(int addr, int pnum, unsigned pram);
	asynStatus sip(int addr, int pnum, epicsInt64 pram);

	asynStatus gsp(int pnum, int maxchar, char* str);

	int P_RUNSTOP;
	int P_UPDATES;
	int P_TS_USEC;
public:
	acq400_asynPortDriver(const char *portName, int maxAddr, int interfaceMask, int interruptMask,
			int asynFlags, int autoConnect, int priority, int stackSize);
	virtual ~acq400_asynPortDriver() {}
};


#endif /* XRMIOCAPP_SRC_ACQ400_ASYNPORTDRIVER_H_ */
