/*
 * acq400_PM.cpp
 *
 *  Created on: 6 Mar 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_PM.h"
#include <fcntl.h>                // open()
#include <unistd.h>
#include <string.h>
static const char *driverName="acq400_PM";

#define DN	driverName
#define FN	__FUNCTION__

acq400_PM::acq400_PM(const char* portName):
	acq400_asynPortDriver(portName,
	/* maxAddr */		MAX_PM_BUFFERS,    /* nbuffers from 0 (most recent) */
	/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
				asynInt8ArrayMask|asynInt16ArrayMask|asynInt32ArrayMask|
				asynFloat32ArrayMask|asynInt64ArrayMask|asynDrvUserMask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
				asynInt8ArrayMask|asynInt16ArrayMask|asynInt32ArrayMask|
				asynFloat32ArrayMask|asynInt64ArrayMask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0)
{
	fprintf(stderr, "%s R1000 \n", FN);

	eventId = epicsEventCreate(epicsEventEmpty);

	createParam(PS_RUNSTOP,  asynParamInt32,        &P_RUNSTOP);
	createParam(PS_UPDATES,  asynParamInt32,        &P_UPDATES);

}

void acq400_PM::task_runner(void *drvPvt)
{
	((acq400_PM *)drvPvt)->task();
}

void acq400_PM::task()
{
	asynStatus status = asynSuccess;
	epicsEventWait(eventId);

	int fc = open("/dev/acq400.0.bq", O_RDONLY);
	assert(fc >= 0);


	if ((ib = getBufferId(fc)) < 0){
		fprintf(stderr, "ERROR: getBufferId() fail");
		return;
	}

	for (int runstop, runstop0 = 0; (ib = getBufferId(fc)) >= 0; runstop0 = runstop){
		lock();
		status = getIntegerParam(P_RUNSTOP, &runstop);
		if (status){
			fprintf(stderr, "%s:%s getIntegerParam P_FMT_MC_PORT fail\n", DN, FN);
			return;
		}
		unlock();
		if (runstop == 1){
			// copy buffer and queue
		}else{
			if (runstop0 == 1){
				// @@todo on stop actions: freeze
				lock();
				// @@todo update callbacks
				unlock();
			}
		}
	}

	close(fc);
}

asynStatus acq400_PM::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *paramName;
    int addr;

    /* Fetch the parameter string name for possible use in debugging */
    getParamName(function, &paramName);

    status = pasynManager->getAddr(pasynUser, &addr);
    if(status!=asynSuccess) return status;

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(addr, function, value);

    fprintf(stderr,
		      "%s:%s: function=%d, addr=%d, name=%s, value=%d\n",
		      DN, FN, function, addr, paramName, value);

    if (function == P_RUNSTOP) {
	if (value) epicsEventSignal(eventId);
    }

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks();

    if (status)
	epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
		  "%s:%s: status=%d, function=%d, name=%s, value=%d",
		  DN, FN, status, function, paramName, value);
    else
	asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
	      "%s:%s: function=%d, name=%s, value=%d\n",
	      DN, FN, function, paramName, value);
    return status;
}


extern "C" {
	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_PM_Configure(const char *portName)
	{
		printf("%s:%s R1001 %s\n", DN, FN, portName);

		new acq400_PM(portName);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg * const initArgs[] = { &initArg0 };
	static const iocshFuncDef initFuncDef = { "acq400_PM_Configure", 1, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		acq400_PM_Configure(args[0].sval);
	}

	void acq400_PM_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_PM_Register);
}

