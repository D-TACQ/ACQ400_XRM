/*
 * acq400_asynPortDriver.cpp
 *
 *  Created on: 6 Mar 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"

static const char *driverName="acq400_asynPortDriver";

#define DN	driverName
#define FN	__FUNCTION__


acq400_asynPortDriver::acq400_asynPortDriver(const char *portName, int maxAddr, int interfaceMask, int interruptMask,
		int asynFlags, int autoConnect, int priority, int stackSize):
        asynPortDriver(portName, maxAddr, interfaceMask, interruptMask,
                   asynFlags, autoConnect, priority, stackSize)
{
	createParam(PS_RUNSTOP,  asynParamInt32,        &P_RUNSTOP);
	createParam(PS_UPDATES,  	asynParamInt32,		&P_UPDATES);
	createParam(PS_TS_USEC,  	asynParamInt64,		&P_TS_USEC);
}

asynStatus acq400_asynPortDriver::gip(int pnum, int* pram)
{
	asynStatus status = getIntegerParam(pnum, pram);
	if (status){
		fprintf(stderr, "%s:%s getIntegerParam %d fail\n",
				DN, FN, pnum);
		assert(status == 0);
	}
	return status;
}
asynStatus acq400_asynPortDriver::gip(int addr, int pnum, int* pram)
{
	asynStatus status = getIntegerParam(addr, pnum, pram);
	if (status){
		fprintf(stderr, "%s:%s:%d getIntegerParam %d fail\n",
				DN, FN, addr, pnum);
		assert(status == 0);
	}
	return status;
}

asynStatus acq400_asynPortDriver::sip(int addr, int pnum, int pram)
{
	asynStatus status = setIntegerParam(addr, pnum, pram);
	if (status){
		fprintf(stderr, "%s:%s:%d setIntegerParam %d fail\n",
				DN, FN, addr, pnum);
		assert(status == 0);
	}
	return status;
}

asynStatus acq400_asynPortDriver::sip(int addr, int pnum, unsigned pram)
{
	asynStatus status = setIntegerParam(addr, pnum, pram);
	if (status){
		fprintf(stderr, "%s:%s:%d setIntegerParam %d fail\n",
				DN, FN, addr, pnum);
		assert(status == 0);
	}
	return status;
}

asynStatus acq400_asynPortDriver::sip(int addr, int pnum, epicsInt64 pram)
{
	asynStatus status = setInteger64Param(addr, pnum, pram);
	if (status){
		fprintf(stderr, "%s:%s:%d setInteger64Param %d fail\n",
				DN, FN, addr, pnum);
		assert(status == 0);
	}
	return status;
}

asynStatus acq400_asynPortDriver::gsp(int pnum, int maxchar, char* str)
{
	asynStatus status = getStringParam(pnum, 80, str);
	if (status){
		fprintf(stderr, "%s:%s SOE_AGG_SITES fail\n", DN, FN);
		assert(status==0);
	}
	return status;
}

